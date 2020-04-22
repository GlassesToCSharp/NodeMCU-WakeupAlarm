#include <ArduinoJson.h>
#include <ESP8266Ping.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

// Uncomment the line below to see the output to the serial port. Output to the
// serial port should be disabled when not in Debug mode.
//#define DEBUG

#include "http_handler.h"
#include "led_handler.h"
#include "lights_handler.h"
#include "string_manipulation.h"
#include "pinouts.h"
#include "wifi_credentials.h"

extern "C" {
#include "user_interface.h"
}

void printAvailableMemory(int counter = -1) {
#ifdef DEBUG
  if (counter >= 0) {
    Serial.print(counter);
  }
  Serial.print(" Free memory: ");
  Serial.println(system_get_free_heap_size());
#endif
}

WiFiServer server(80);

// The URL adderss cannot have "000" in the IP address. This needs to be
// determined later.
String getUrl = "http://192.168.001.000:3000/conf/wakeup";
// Set the query time as 2am
const uint8_t queryHour = 2;
const uint8_t queryMinute = 0;
const uint32_t millisecondsSwitchedOnFactor = 60000; // 1 min

const LED_COLORS alarmColor = { .red = 1023, .green = 1023, .blue = 0}; // In RGB format, 0-1023
const LED_COLORS lightsOff = {0, 0, 0}; // Lights off!
LED_COLORS currentLightColor = {0, 0, 0};

// 4 objects, one is an object containing two objects. Add 52 bytes to it for strings duplication.
// See https://arduinojson.org/v6/assistant/ for more information. Example JSON expected to receive:
//  {
//      "currentTime": {
//          "hour": 12,
//          "minute": 28
//      },
//      "hour": 16,
//      "minute": 30,
//      "duration": 10,
//      "active": true
//  }
const size_t capacity = JSON_OBJECT_SIZE(2) + JSON_OBJECT_SIZE(5) + 52;
StaticJsonDocument<capacity> doc;
DeserializationError deserializationError;

// Set the current time to be 12:00pm (midday)
int currentHour = 12;
int currentMinute = 00;
int alarmHour = 12;
int alarmMinute = 00;
int alarmDuration = 10; // Default value
bool alarmActive = false;
bool enableAlarm = true;

uint64_t elapsedTime = 0;
bool alarmDataRequested = false;

void handleConnectionStatus();
void getJsonAndHandleResponse();
void handleWaitingUntilAlarmTime();
void handleLocalHtmlQuery();
void handleSetColourRequest();
void handleEnableAlarmRequest();
bool handleGetConfigurationRequest();

void setup() {
  // put your setup code here, to run once:
#ifdef DEBUG
  Serial.begin(115200);
  delay(10);
#endif
  initialiseLedHandler();
  initialiseLights();
#ifdef DEBUG
  Serial.println("Testing light...");
#endif
  delay(5000);
  setLedHandlerState(STATE_CONNECTING);

  // Connect to WiFi network
#ifdef DEBUG
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
#endif

  WiFi.begin(ssid, password);

  handleConnectionStatus();

#ifdef DEBUG
  Serial.println("");
  Serial.println("WiFi connected");
#endif

  // Initialise server
  server.begin();
#ifdef DEBUG
  Serial.println("Server started");
#endif

  // Print the IP address
  IPAddress pLocalIp = WiFi.localIP();
#ifdef DEBUG
  Serial.print("Use this URL to connect: ");
  Serial.print("http://");
  Serial.print(pLocalIp);
  Serial.println("/");
#endif

  // TODO: Auto detect the correct IP address of the server.
  // Try ping-ing every IP address on the LAN for a matching
  // result. If requesting alarm data from the pinged address
  // works, this is the correct address.
  // Start the detection at x.x.x.2. IP addresses never start
  // at 0, and the first address is the router's address.
  for (uint8_t i = 2; i < 255; i++) {
    IPAddress *pIp = new IPAddress(192, 168, 1, i);
    // If a new instance has not been created, fail the entire
    // process.
    if (pIp == NULL) {
      setLedHandlerState(STATE_FAILED);
      setBoardLedState(ON);
      while (1) {
        delay(2000);
      }
      // Do not pass this point until reset.
    }
    
    if ((pLocalIp != *pIp) && Ping.ping(*pIp)) {
#ifdef DEBUG
      Serial.print("Ping successful: ");
      Serial.println((*pIp).toString());
#endif
      // Assign IP address value.
      if (i >= 100) {
        getUrl[19] = (char)((uint8_t)(i / 100)) + '0';
      }
      if (i >= 10) {
        getUrl[20] = (char)((uint8_t)((i % 100) / 10)) + '0';
      }
      getUrl[21] = (char)((uint8_t)(i % 10)) + '0';
      
      // Get the JSON response with alarm data.
#ifdef DEBUG
      Serial.print("Requesting to: ");
      Serial.println(getUrl);
#endif
      getJsonAndHandleResponse();
      
      // Check that data has been received.
      if (alarmDataRequested) {
        delete pIp;
        pIp = NULL;
        break;
      } else {
        getUrl[19] = '0';
        getUrl[20] = '0';
        getUrl[21] = '0';
      }
    }

    delete pIp;
    pIp = NULL;
  }

#ifdef DEBUG
  Serial.println("Server URL to use:");
  Serial.println(getUrl);
#endif

  printAvailableMemory();
}

void loop() {
  // Wait until the right amount of time has elapsed before querying the server again.
  handleWaitingUntilAlarmTime();
  
  alarmDataRequested = false;
  getJsonAndHandleResponse();
}

void handleConnectionStatus() {
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
#ifdef DEBUG
    Serial.print(".");
#endif
    if (WiFi.status() == WL_CONNECT_FAILED) {
      setLedHandlerState(STATE_FAILED);
      setBoardLedState(ON);
      while (1) {
        delay(2000);
      }
    }
  }
  setLedHandlerState(STATE_CONNECTED);
  // Set colour off.
  setLightColor(lightsOff);
}

void getJsonAndHandleResponse() {
  String json = httpGet(getUrl);
  if (json == "") {
#ifdef DEBUG
    Serial.println(F("Failed to get json"));
#endif
    setBoardLedState(ON);
    return;
  }

#ifdef DEBUG
  Serial.println(json);
#endif
  deserializationError = deserializeJson(doc, json);
  if (deserializationError) {
#ifdef DEBUG
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(deserializationError.c_str());
#endif
    setBoardLedState(ON);
  } else {
    alarmHour = doc["hour"];
    alarmMinute = doc["minute"];
    alarmDuration = doc["duration"];
    alarmActive = doc["active"];
    currentHour = doc["currentTime"]["hour"];
    currentMinute = doc["currentTime"]["minute"];
#ifdef DEBUG
    Serial.print("Current time: ");
    Serial.print(currentHour);
    Serial.print(":");
    Serial.println(currentMinute);
    Serial.print("Hour: ");
    Serial.println(alarmHour);
    Serial.print("Minute: ");
    Serial.println(alarmMinute);
    Serial.print("Duration: ");
    Serial.println(alarmDuration);
    Serial.print("Active: ");
    Serial.println(alarmActive);
#endif
    setBoardLedState(OFF);
    alarmDataRequested = true;
  }
}

void handleWaitingUntilAlarmTime() {
  elapsedTime = millis();
  long lightsLitTime = 0;
  while ((alarmDataRequested) || (currentHour != queryHour) || (currentMinute != queryMinute)) {
    if (millis() - elapsedTime >= 60000) {
      elapsedTime = millis();
      currentMinute++;
      // Reset this flag here, as it must be reset when the time changes.
      alarmDataRequested = false;
    }

    if (currentMinute >= 60) {
      currentHour++;
      currentHour = currentHour % 24; // we're not interested in the actual date
      currentMinute = currentMinute % 60;
    }

    // Every 10 seconds, check the time and lights status. Otherwise, check for inbound connections.
    long serverCounterTime = millis();
    while (millis() - serverCounterTime < 10000) {
      handleLocalHtmlQuery();
    }

    if ((enableAlarm) && 
        (alarmActive) && 
        (currentHour == alarmHour) && 
        (currentMinute == alarmMinute) &&
        (lightsLitTime == 0)) {
      // Turn lights on if the alarm is active and the hour and minute values match the alarm values.
#ifdef DEBUG
      Serial.println(F("Activating lights!"));
#endif
      setLightColor(alarmColor);
      currentLightColor = alarmColor;
      lightsLitTime = millis();
    } else if ((lightsLitTime > 0) && (millis() - lightsLitTime > (alarmDuration * millisecondsSwitchedOnFactor))) {
      // Turn the lights off if they are turned of (determined by lightLitTime being > 0) and at least 10mins has elapsed since turned on.
#ifdef DEBUG
      Serial.println(F("Turning lights off."));
#endif
      setLightColor(lightsOff);
      currentLightColor = lightsOff;
      lightsLitTime = 0;
    }
  }
}


const uint16_t requestUrlBufferSize = 128;
char requestUrlBuffer[requestUrlBufferSize];
void handleLocalHtmlQuery() {
  // Check if a client has connected
  client = server.available();
  if (!client) {
    return;
  }

  // Wait until the client sends some data
#ifdef DEBUG
  Serial.println("new client");
#endif
  while (!client.available()) {
    delay(1);
  }

  // Read the first line of the request
  memset(requestUrlBuffer, 0, requestUrlBufferSize);
  int bytesRead = client.readBytesUntil('\r', requestUrlBuffer, requestUrlBufferSize);
  Serial.println(requestUrlBuffer);
  client.flush();

  if (bytesRead == 0) {
    return;
  }
  
  printAvailableMemory(1);
  
  // Check if the colour has been set
  // /setColor?lightsColor=#000000
  handleSetColourRequest();
  
  printAvailableMemory(5);
  
  // Check if the alarm enabled has been set
  // /alarm?enable=1
  handleEnableAlarmRequest();

  printAvailableMemory(9);
  // Check if we are received a configuration request
  // /getConfiguration
  if (handleGetConfigurationRequest()) {
    respondWithJsonContent(currentHour, currentMinute, 
      alarmHour, alarmMinute, enableAlarm, alarmActive, &currentLightColor);
  } else {
    generateDiagnosticHtmlContent(currentHour, currentMinute, 
      alarmHour, alarmMinute, enableAlarm, alarmActive, &currentLightColor);
  }
  
  printAvailableMemory(10);
  client.println(htmlStringBuffer);
  printAvailableMemory(11);
}

void handleSetColourRequest() {
  strcpy_P(inputString1, requestUrlBuffer);
  strcpy_P(inputString2, apiSetColor);
  int indexOfColorRequest = basicInstr(inputString1, inputString2);
  if (indexOfColorRequest != -1) {
    // Begin less long-ass process of extracting the colour parameter from the query.
    // Get the starting index of the searched string. Don't include the rest of the
    // searched string (13-chars long). The string will also contain the URL encoded
    // value for '#' (%23, 3-chars long). Dont include it.
    strcpy_P(inputString2, apiSetColorParamQuestion);
    int indexOfColorValueQuestion = basicInstr(inputString1, inputString2) + (sizeof(apiSetColorParamQuestion) - 1) + 3;
    strcpy_P(inputString2, apiSetColorParamAmper);
    int indexOfColorValueAmpersand = basicInstr(inputString1, inputString2) + (sizeof(apiSetColorParamAmper) - 1) + 3;
    
    printAvailableMemory(2);
    char* colorRequest;
    if (indexOfColorValueQuestion >= 16) {
      // Handle getting first query parameter
      colorRequest = basicMidString(inputString1, indexOfColorValueQuestion, 7);
    } else if (indexOfColorValueAmpersand >= 16) {
      // Handle getting the nth query parameter
      colorRequest = basicMidString(inputString1, indexOfColorValueAmpersand, 7);
    } else {
      // Doesn't exist.
    }
    printAvailableMemory(3);

    if (colorRequest[0] != '\0') {
      LED_COLORS color = {0, 0, 0};
      for (int colourIndex = 0; colourIndex < 3; colourIndex++) {
        uint8_t intValue = 0;
        uint8_t digit = 0;
        
        for (uint8_t i = 0; i < 2; i++) {
          uint8_t stringIndex = (2 * colourIndex) + i;
          if ((colorRequest[stringIndex] >= '0') && (colorRequest[stringIndex] <= '9')) {
            digit = colorRequest[stringIndex] - '0';
          } else if (((colorRequest[stringIndex] >= 'A') && (colorRequest[stringIndex] <= 'F')) || 
                     ((colorRequest[stringIndex] >= 'a') && (colorRequest[stringIndex] <= 'f'))) {
            switch (colorRequest[stringIndex])   {
              case 'A': case 'a': digit = 10; break;
              case 'B': case 'b': digit = 11; break;
              case 'C': case 'c': digit = 12; break;
              case 'D': case 'd': digit = 13; break;
              case 'E': case 'e': digit = 14; break;
              case 'F': case 'f': digit = 15; break;
            }
          }

          intValue = (intValue << (4 * i)) + digit;
        }

        uint16_t colorValue = to10bit(intValue);

        switch (colourIndex) {
          case 0: color.red = colorValue; break;
          case 1: color.green = colorValue; break;
          case 2: color.blue = colorValue; break;
        }
      }

      setLightColor(color);
      currentLightColor = color;
    }
    printAvailableMemory(4);
  }
}

void handleEnableAlarmRequest() {
  strcpy_P(inputString2, apiEnableAlarm);
  int indexOfAlarmRequest = basicInstr(inputString1, inputString2);
  if (indexOfAlarmRequest != -1) {
    // Begin less long-ass process of extracting the colour parameter from the query.
    // Get the starting index of the searched string. Don't include the rest of the
    // searched string (8-chars long). Dont include it.
    strcpy_P(inputString2, apiEnableAlarmParamQuestion);
    int indexOfAlarmValueQuestion = basicInstr(inputString1, inputString2) + (sizeof(apiEnableAlarmParamQuestion) - 1);
    strcpy_P(inputString2, apiEnableAlarmParamAmper);
    int indexOfAlarmValueAmpersand = basicInstr(inputString1, inputString2) + (sizeof(apiEnableAlarmParamAmper) - 1);
    printAvailableMemory(6);
    
    char* alarmOn;
    if (indexOfAlarmValueQuestion >= 8) {
      // Handle getting first query parameter
      alarmOn = basicMidString(inputString1, indexOfAlarmValueQuestion, 2);
    } else if (indexOfAlarmValueAmpersand >= 8) {
      // Handle getting the nth query parameter
      alarmOn = basicMidString(inputString1, indexOfAlarmValueAmpersand, 2);
    } else {
      // Doesn't exist.
    }
    printAvailableMemory(7);
    enableAlarm = alarmOn[0] == '1';
    
    printAvailableMemory(8);
  }
}

bool handleGetConfigurationRequest() {
  strcpy_P(inputString2, apiGetCurrentConfiguration);
  int indexOfConfigurationRequest = basicInstr(inputString1, inputString2);
  // Getting the configuration doesn't require checking for query parameters
  // because there aren't any. Simply acknowledge that a request for the
  // configuration has been made.
  return indexOfConfigurationRequest != -1;
}
