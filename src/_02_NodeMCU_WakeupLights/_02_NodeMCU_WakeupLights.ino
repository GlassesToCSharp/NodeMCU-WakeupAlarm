#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

#include "http_handler.h"
#include "led_handler.h"
#include "lights_handler.h"
#include "pinouts.h"
#include "wifi_credentials.h"

extern "C" {
#include "user_interface.h"
}

void printAvailableMemory(int counter = -1) {
  if (counter >= 0) {
    Serial.print(counter);
  }
  Serial.print(" Free memory: ");
  Serial.println(system_get_free_heap_size());
}

WiFiServer server(80);

const String getUrl = "http://192.168.1.68:3000/conf/wakeup";
// Set the query time as 2am
const uint8_t queryHour = 2;
const uint8_t queryMinute = 0;
const uint32_t millisecondsSwitchedOn = 600000; // 10 mins

const LED_COLORS alarmColor = { .red = 1023, .green = 1023, .blue = 0}; // In RGB format, 0-1023
const LED_COLORS lightsOff = {0, 0, 0}; // Lights off!
LED_COLORS currentLightColor = {0, 0, 0};

// 4 objects, one is an object containing two objects. Add 50 bytes to it.
const size_t capacity = JSON_OBJECT_SIZE(2) + JSON_OBJECT_SIZE(4) + 50;
DynamicJsonDocument doc(capacity);

// Set the current time to be 12:00pm (midday)
int currentHour = 12;
int currentMinute = 00;
int alarmHour = 12;
int alarmMinute = 00;
bool alarmActive = false;
bool enableAlarm = true;

uint64_t elapsedTime = 0;
bool alarmDataRequested = false;

void handleConnectionStatus();
void getJsonAndHandleResponse();
void handleWaitingUntilAlarmTime();
void handleLocalHtmlQuery();
void createHtmlDisplay(const LED_COLORS* currentColor, const bool showDiagnostics);

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  delay(10);
  initialiseLedHandler();
  initialiseLights();
  Serial.println("Testing light...");
  delay(5000);
  setLedHandlerState(STATE_CONNECTING);

  // Connect to WiFi network
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  handleConnectionStatus();

  Serial.println("");
  Serial.println("WiFi connected");

  // Initialise server
  server.begin();
  Serial.println("Server started");

  // Print the IP address
  Serial.print("Use this URL to connect: ");
  Serial.print("http://");
  Serial.print(WiFi.localIP());
  Serial.println("/");

  printAvailableMemory();
}

void loop() {
  // put your main code here, to run repeatedly:
  alarmDataRequested = false;
  getJsonAndHandleResponse();

  // Wait until the right amount of time has elapsed before querying the server again.
  handleWaitingUntilAlarmTime();
}

void handleConnectionStatus() {
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    if (WiFi.status() == WL_CONNECT_FAILED) {
      setLedHandlerState(STATE_FAILED);
      setBoardLedState(false);
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
  if (json != "") {
    Serial.println(json);
    DeserializationError error = deserializeJson(doc, json);
    if (error) {
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(error.c_str());
      setBoardLedState(false);
    } else {
      alarmHour = doc["hour"];
      alarmMinute = doc["minute"];
      alarmActive = doc["active"];
      currentHour = doc["currentTime"]["hour"];
      currentMinute = doc["currentTime"]["minute"];
      Serial.print("Current time: ");
      Serial.print(currentHour);
      Serial.print(":");
      Serial.println(currentMinute);
      Serial.print("Hour: ");
      Serial.println(alarmHour);
      Serial.print("Minute: ");
      Serial.println(alarmMinute);
      Serial.print("Active: ");
      Serial.println(alarmActive);
      setBoardLedState(true);
      alarmDataRequested = true;
    }
  } else {
    Serial.println("Failed to get json");
    setBoardLedState(false);
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

    long serverCounterTime = millis();
    while (millis() - serverCounterTime < 10000) {
      handleLocalHtmlQuery();
    }

    if ((enableAlarm) && 
        (alarmActive) && 
        (currentHour == alarmHour) && 
        (currentMinute == alarmMinute)) {
      // Turn lights on if the alarm is active and the hour and minute values match the alarm values.
      Serial.println("Activating lights!");
      setLightColor(alarmColor);
      currentLightColor = alarmColor;
      lightsLitTime = millis();
    } else if ((lightsLitTime > 0) && (millis() - lightsLitTime > millisecondsSwitchedOn)) {
      // Turn the lights off if they are turned of (determined by lightLitTime being > 0) and at least 10mins has elapsed since turned on.
      Serial.println("Turning lights off.");
      setLightColor(lightsOff);
      currentLightColor = lightsOff;
      lightsLitTime = 0;
    }
  }
}

void handleLocalHtmlQuery() {
  // Check if a client has connected
  client = server.available();
  if (!client) {
    return;
  }

  // Wait until the client sends some data
  Serial.println("new client");
  while (!client.available()) {
    delay(1);
  }

  // Read the first line of the request
  String request = client.readStringUntil('\r');
  Serial.println(request);
  client.flush();
  
  printAvailableMemory(1);
  
  // Check if the colour has been set **BEFORE CREATING THE WEB PAGE**
  int indexOfColorRequest = request.indexOf("/setColor");
  if (indexOfColorRequest != -1) {
    // Begin less long-ass process of extracting the colour parameter from the query.
    // This process uses String commands. Eurgh.
    // The `indexOf` function return the starting index of the searched string. Don't
    // include the rest of the searched string (13-chars long). The string will also
    // contain the URL encoded value for '#' (%23, 3-chars long). Dont include it.
    int indexOfColorValueQuestion = request.indexOf("?lightsColor=") + 13 + 3;
    int indexOfColorValueAmpersand = request.indexOf("&lightsColor=") + 13 + 3;
  printAvailableMemory(2);
    String colorRequest;
    if (indexOfColorValueQuestion != -1) {
      // Handle getting first query parameter
      colorRequest = request.substring(indexOfColorValueQuestion, indexOfColorValueQuestion + 6);
    } else if (indexOfColorValueAmpersand != -1) {
      // Handle getting the nth query parameter
      colorRequest = request.substring(indexOfColorValueAmpersand, indexOfColorValueAmpersand + 6);
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
          if (colorRequest[stringIndex] >= '0' && colorRequest[stringIndex] <= '9') {
            digit = colorRequest[stringIndex] - '0';
          } else if ((colorRequest[stringIndex] >= 'A' && colorRequest[stringIndex] <= 'F') || 
                     (colorRequest[stringIndex] >= 'a' && colorRequest[stringIndex] <= 'f')) {
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
  
  printAvailableMemory(5);
  
  // Check if the colour has been set **BEFORE CREATING THE WEB PAGE**
  int indexOfAlarmRequest = request.indexOf("/alarm");
  if (indexOfAlarmRequest != -1) {
    // Begin less long-ass process of extracting the colour parameter from the query.
    // This process uses String commands. Eurgh.
    // The `indexOf` function return the starting index of the searched string. Don't
    // include the rest of the searched string (8-chars long). Dont include it.
    int indexOfAlarmValueQuestion = request.indexOf("?enable=") + 8;
    int indexOfAlarmValueAmpersand = request.indexOf("&enable=") + 8;
  printAvailableMemory(6);
    String alarmOn = alarmOn ? String("on") : String("  ");
    if (indexOfAlarmValueQuestion != -1) {
      // Handle getting first query parameter
      alarmOn = request.substring(indexOfAlarmValueQuestion, indexOfAlarmValueQuestion + 2);
    } else if (indexOfAlarmValueAmpersand != -1) {
      // Handle getting the nth query parameter
      alarmOn = request.substring(indexOfAlarmValueAmpersand, indexOfAlarmValueAmpersand + 2);
    } else {
      // Doesn't exist.
    }
  printAvailableMemory(7);

    enableAlarm = alarmOn.equalsIgnoreCase("on");
    
  printAvailableMemory(8);
  }

  printAvailableMemory(9);
  generateDiagnosticHtmlContent(currentHour, currentMinute, alarmHour, alarmMinute, enableAlarm, &currentLightColor);

  printAvailableMemory(10);
  client.println(htmlStringBuffer);
  printAvailableMemory(11);
}
