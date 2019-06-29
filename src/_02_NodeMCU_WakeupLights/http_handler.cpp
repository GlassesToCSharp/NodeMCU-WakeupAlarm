#include <Arduino.h>
#include <ArduinoJson.h>
#include <ESP8266HTTPClient.h>

#include "lights_handler.h"

const char diagnosticHtml[] PROGMEM = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<!DOCTYPE HTML>\r\n<html>\r\n<br>\r\nTime: %02d:%02d\r\n<br>\r\nAlarm: %02d:%02d\r\n<br>\r\nAlarm enabled: %d\r\n<br>\r\nActive: %d\r\n<br>\r\nLights colour: %s\r\n<br>\r\n</html>\r\n";

// These arrays should prevent having to assign new memory every time the HTML code is generated
const uint16_t htmlStringBufferSize = 512;
char htmlStringBuffer[htmlStringBufferSize]; 
const uint16_t lightsColorBufferSize = 16;
char lightsColorBuffer[lightsColorBufferSize];

const uint8_t resetBufferValue = 0;

char apiSetColor[10] PROGMEM = "/setColor";
char apiSetColorParamQuestion[14] PROGMEM = "?lightsColor=";
char apiSetColorParamAmper[14] PROGMEM = "&lightsColor=";

char apiEnableAlarm[7] PROGMEM = "/alarm";
char apiEnableAlarmParamQuestion[9] PROGMEM = "?enable=";
char apiEnableAlarmParamAmper[9] PROGMEM = "&enable=";

char apiGetCurrentConfiguration[18] PROGMEM = "/getConfiguration";

// The additional 192 is required according to the documentation.
const size_t serializedJsonCapacity = 2*JSON_OBJECT_SIZE(2) + JSON_OBJECT_SIZE(3) + JSON_OBJECT_SIZE(5) + 192;
DynamicJsonDocument serializedJson(serializedJsonCapacity);

HTTPClient http;
WiFiClient client;

// I don't like using String...
String httpGet(const String url, bool debug = false) {
  // End the connecttion even if never started.
  http.end();
  // Begin a new connection.
  http.begin(client, url);
  int httpResponseCode = http.GET();
  if (httpResponseCode > 0) {
    // The request was successful, but it may be a 404 or sumat...
    return http.getString();
  } else {
    // Likely the endpoint could not be reached.
    return "";
  }
}

void generateDiagnosticHtmlContent(const uint8_t currentHour, const uint8_t currentMinute, 
    const uint8_t alarmHour, const uint8_t alarmMinute, const bool isAlarmEnabled, 
    const bool isAlarmActive, const LED_COLORS* lightsColor) {
  memset(htmlStringBuffer, resetBufferValue, htmlStringBufferSize);
  memset(lightsColorBuffer, resetBufferValue, lightsColorBufferSize);
  colorsToCharArray(lightsColorBuffer, lightsColor);
  sprintf(htmlStringBuffer, diagnosticHtml, currentHour, currentMinute, alarmHour, alarmMinute, isAlarmEnabled ? 1 : 0, isAlarmActive ? 1 : 0, lightsColorBuffer);
}

// Sends in the following JSON format:
//  {
//    "currentTime": {
//      "hour": 12,
//      "minute": 10
//    },
//    "alarmTime": {
//      "hour": 12,
//      "minute": 10
//    },
//    "enable": 1,
//    "active": 1,
//    "lightColour": {
//      "red": 255,
//      "green": 255,
//      "blue": 255
//    }
//  }
void respondWithJsonContent(const uint8_t currentHour, const uint8_t currentMinute, 
    const uint8_t alarmHour, const uint8_t alarmMinute, const bool isAlarmEnabled, 
    const bool isAlarmActive, const LED_COLORS* lightsColor) {  
  // The additional 192 is required according to the documentation.
//  const size_t serializedJsonCapacity = 2*JSON_OBJECT_SIZE(2) + JSON_OBJECT_SIZE(3) + JSON_OBJECT_SIZE(5) + 192;
//  DynamicJsonDocument serializedJson(serializedJsonCapacity);
  
  JsonObject currentTime = serializedJson.createNestedObject("currentTime");
  currentTime["hour"] = currentHour;
  currentTime["minute"] = currentMinute;
  
  JsonObject alarmTime = serializedJson.createNestedObject("alarmTime");
  alarmTime["hour"] = alarmHour;
  alarmTime["minute"] = alarmMinute;
  serializedJson["enable"] = isAlarmEnabled ? 1 : 0;
  serializedJson["active"] = isAlarmActive ? 1 : 0;
  
  JsonObject lightColour = serializedJson.createNestedObject("lightColour");
  lightColour["red"] = to8bit(lightsColor->red);
  lightColour["green"] = to8bit(lightsColor->green);
  lightColour["blue"] = to8bit(lightsColor->blue);
  
  serializeJson(serializedJson, client);
}
