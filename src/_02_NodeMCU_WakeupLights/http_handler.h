#include "lights_handler.h"

const char html[] PROGMEM = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<!DOCTYPE HTML>\r\n<html>\r\n<form action=\"/setColor\">Select lights color:<input type=\"color\" name=\"lightsColor\" value=\"#%s\"><input type=\"submit\"></form>\r\n<a href=\"/diagnostics\"\"><button>Get diagnostics</button></a>\r\n%s</html>\r\n";

const char diagnosticHtml[] PROGMEM = "<br>\r\nTime: %02d:%02d\r\n<br>\r\nAlarm: %02d:%02d\r\n<br>\r\nActive: %d\r\n<br>\r\nLights on: %d\r\n<br>\r\n";

HTTPClient http;

// I don't like using String...
String httpGet(const String url, bool debug = false) {
  // End the connecttion even if never started.
  http.end();
  // Begin a new connection.
  http.begin(url);
  int httpResponseCode = http.GET();
  if (httpResponseCode > 0) {
    // The request was successful, but it may be a 404 or sumat...
    return http.getString();
  } else {
    // Likely the endpoint could not be reached.
    return "";
  }
}

char* generateDiagnosticHtmlContent(uint8_t currentHour, uint8_t currentMinute, uint8_t alarmHour, uint8_t alarmMinute, bool isAlarmActive, bool areLightsOn) {
  static char *htmlString = (char*) malloc(strlen(diagnosticHtml) + 1);
  sprintf(htmlString, diagnosticHtml, currentHour, currentMinute, alarmHour, alarmMinute, isAlarmActive ? 1 : 0, areLightsOn ? 1 : 0);
  return htmlString;
}

char* generateFullHtmlContent(const LED_COLORS* currentColor, const char* diagnosticHtml) {
  static char *htmlString = (char*) malloc(strlen(html) + strlen(diagnosticHtml) + 1);
  char currentColorChars[7];
  colorsToCharArray(currentColorChars, currentColor);
  sprintf(htmlString, (char*)html, currentColorChars, diagnosticHtml);
  return htmlString;
}
