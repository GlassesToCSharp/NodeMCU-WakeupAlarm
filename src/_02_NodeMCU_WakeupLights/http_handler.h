#include "lights_handler.h"

extern String httpGet(const String url, bool debug = false);
extern char* generateDiagnosticHtmlContent(uint8_t currentHour, uint8_t currentMinute, uint8_t alarmHour, uint8_t alarmMinute, bool isAlarmActive, bool areLightsOn);
extern char* generateFullHtmlContent(const LED_COLORS* currentColor, const char* diagnosticHtml, bool isAlarmEnabled);
