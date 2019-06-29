#include "lights_handler.h"
#include <ESP8266HTTPClient.h>

extern char htmlStringBuffer[512];
extern char apiSetColor[10];
extern char apiSetColorParamQuestion[14];
extern char apiSetColorParamAmper[14];
extern char apiEnableAlarm[7];
extern char apiEnableAlarmParamQuestion[9];
extern char apiEnableAlarmParamAmper[9];
extern char apiGetCurrentConfiguration[18];
extern WiFiClient client;

extern String httpGet(const String url, bool debug = false);
extern void generateDiagnosticHtmlContent(const uint8_t currentHour, const uint8_t currentMinute, 
    const uint8_t alarmHour, const uint8_t alarmMinute, const bool isAlarmEnabled, 
    const bool isAlarmActive, const LED_COLORS* lightsColor);
extern void respondWithJsonContent(const uint8_t currentHour, const uint8_t currentMinute, 
    const uint8_t alarmHour, const uint8_t alarmMinute, const bool isAlarmEnabled, 
    const bool isAlarmActive, const LED_COLORS* lightsColor);
