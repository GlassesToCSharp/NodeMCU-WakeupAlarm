#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

#include "http_handler.h"
#include "led_handler.h"
#include "lights_handler.h"
#include "pinouts.h"
#include "wifi_credentials.h"

const String getUrl = "http://192.168.1.68:3000/conf/wakeup";
// Set the query time as 2am
const uint8_t queryHour = 2;
const uint8_t queryMinute = 0;
const uint32_t millisecondsSwitchedOn = 600000; // 10 mins

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
  setLightColor(0, 0, 0);
  
  Serial.println("");
  Serial.println("WiFi connected");
 
  // Print the IP address
  Serial.print("Use this URL to connect: ");
  Serial.print("http://");
  Serial.print(WiFi.localIP());
  Serial.println("/");
}

void loop() {
  // put your main code here, to run repeatedly:
  alarmDataRequested = false;
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

  // Wait until the right amount of time has elapsed before querying the server again.
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

    delay(10000);

    if ((alarmActive) && (currentHour == alarmHour) && (currentMinute == alarmMinute)) {
      // Turn lights on if the alarm is active and the hour and minute values match the alarm values.
      Serial.println("Activating lights!");
      setLightColor(1023, 1023, 0);
      lightsLitTime = millis();
    } else if ((lightsLitTime > 0) && (millis() - lightsLitTime > millisecondsSwitchedOn)) {
      // Turn the lights off if they are turned of (determined by lightLitTime being > 0) and at least 10mins has elapsed since turned on.
      Serial.println("Turning lights off.");
      setLightColor(0, 0, 0);
      lightsLitTime = 0;
    }

    Serial.print("Estimated time: ");
    Serial.print(currentHour);
    Serial.print(":");
    Serial.println(currentMinute);
  }
}
