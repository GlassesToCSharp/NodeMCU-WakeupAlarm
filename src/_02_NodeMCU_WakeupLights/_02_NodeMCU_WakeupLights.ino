#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

#include "http_handler.h"
#include "led_handler.h"
#include "lights_handler.h"
#include "pinouts.h"
#include "wifi_credentials.h"

WiFiServer server(80);

const String getUrl = "http://192.168.1.68:3000/conf/wakeup";
// Set the query time as 2am
const uint8_t queryHour = 2;
const uint8_t queryMinute = 0;
const uint32_t millisecondsSwitchedOn = 600000; // 10 mins

const LED_COLORS alarmColor = { .red = 1023, .green = 1023, .blue = 0}; // In RGB format, 0-1023
const LED_COLORS lightsOff = {0, 0, 0}; // Lights off!
// 4 objects, one is an object containing two objects. Add 50 bytes to it.
const size_t capacity = JSON_OBJECT_SIZE(2) + JSON_OBJECT_SIZE(4) + 50;
DynamicJsonDocument doc(capacity);

// Set the current time to be 12:00pm (midday)
int currentHour = 12;
int currentMinute = 00;
int alarmHour = 12;
int alarmMinute = 00;
bool alarmActive = false;

uint64_t elapsedTime = 0;
bool alarmDataRequested = false;

void handleConnectionStatus();
void getJsonAndHandleResponse();
void handleWaitingUntilAlarmTime();
void handleLocalHtmlQuery();

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
      while(1){
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
  while((alarmDataRequested) || (currentHour != queryHour) || (currentMinute != queryMinute)) {
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
    while(millis() - serverCounterTime < 10000) {
      handleLocalHtmlQuery(lightsLitTime > 0);
    }

    if ((alarmActive) && (currentHour == alarmHour) && (currentMinute == alarmMinute)) {
      // Turn lights on if the alarm is active and the hour and minute values match the alarm values.
      Serial.println("Activating lights!");
      setLightColor(alarmColor);
      lightsLitTime = millis();
    } else if ((lightsLitTime > 0) && (millis() - lightsLitTime > millisecondsSwitchedOn)) {
      // Turn the lights off if they are turned of (determined by lightLitTime being > 0) and at least 10mins has elapsed since turned on.
      Serial.println("Turning lights off.");
      setLightColor(lightsOff);
      lightsLitTime = 0;
    }
  }
}

void handleLocalHtmlQuery(bool areLightsOn) {
  // Check if a client has connected
  WiFiClient client = server.available();
  if (!client) {
    return;
  }
 
  // Wait until the client sends some data
  Serial.println("new client");
  while(!client.available()){
    delay(1);
  }
 
  // Read the first line of the request
  String request = client.readStringUntil('\r');
  Serial.println(request);
  client.flush();
 
  // Return the response
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html");
  client.println(""); //  do not forget this one
  client.println("<!DOCTYPE HTML>");
  client.println("<html>");

  client.println("<a href=\"/diagnostics\"\"><button>Get diagnostics</button></a>");
  // Check if we have asked for the diagnostics.
  if (request.indexOf("/diagnostics") != -1) {
    char lineBreak[] = "<br>";
    client.print(lineBreak);
    client.println(lineBreak);
    char currentTime[12];
    char alarmTime[13];
    char alarmActive[10];
    char lightsOn[13];
    snprintf(currentTime, 12, "Time: %02d:%02d", currentHour, currentMinute);
    snprintf(alarmTime, 13, "Alarm: %02d:%02d", alarmHour, alarmMinute);
    snprintf(alarmActive, 10, "Active: %d", alarmActive ? 1 : 0);
    snprintf(lightsOn, 13, "Lights on: %d", areLightsOn ? 1 : 0);
    client.println(currentTime);
    client.println(lineBreak);
    client.println(alarmTime);
    client.println(lineBreak);
    client.println(alarmActive);
    client.println(lineBreak);
    client.println(lightsOn);
    client.println(lineBreak);
  }
  client.println("</html>");
}
