#include "lights_handler.h"

extern char htmlStringBuffer[512];

extern String httpGet(const String url, bool debug = false);
extern void generateDiagnosticHtmlContent(uint8_t currentHour, uint8_t currentMinute, uint8_t alarmHour, uint8_t alarmMinute, bool isAlarmActive, const LED_COLORS* lightColor);
