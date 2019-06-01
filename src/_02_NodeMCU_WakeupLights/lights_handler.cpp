#include <Arduino.h>

#include "pinouts.h"

#define RED_LIGHTS D1
#define GREEN_LIGHTS D2
#define BLUE_LIGHTS D5

typedef struct {
  uint16_t red;
  uint16_t green;
  uint16_t blue;
} LED_COLORS;

bool operator==(const LED_COLORS& first, const LED_COLORS& second) {
  return first.red == second.red &&
    first.green == second.green &&
    first.blue == second.blue;
}

bool operator!=(const LED_COLORS& first, const LED_COLORS& second) {
  return !(first == second);
}

void initialiseLights() {
  pinMode(RED_LIGHTS, OUTPUT);
  pinMode(GREEN_LIGHTS, OUTPUT);
  pinMode(BLUE_LIGHTS, OUTPUT);
}

void setLightColor(uint16_t red, uint16_t green, uint16_t blue) {
  analogWrite(RED_LIGHTS, 1023 - red);
  analogWrite(GREEN_LIGHTS, 1023 - green);
  analogWrite(BLUE_LIGHTS, 1023 - blue);
}

void setLightColor(const LED_COLORS lights) {
  setLightColor(lights.red, lights.green, lights.blue);
}

uint8_t to8bit(uint16_t color) {
  return map(color, 0, 1023, 0, 255);
}

uint16_t to10bit(uint8_t color) {
  return map(color, 0, 255, 0, 1023);
}

void colorsToCharArray(char* charArray, const LED_COLORS* color, uint8_t desiredLength = 7, bool as8bitColors = true, bool asHex = true) {
  if (as8bitColors) {
    if (asHex) {
      // Default option
      snprintf(charArray, desiredLength, "%02x%02x%02x", to8bit((*color).red), to8bit((*color).green), to8bit((*color).blue));
    } else {
      snprintf(charArray, desiredLength, "%3d, %3d, %3d", to8bit((*color).red), to8bit((*color).green), to8bit((*color).blue));
    }
  } else {
    if (asHex) {
      snprintf(charArray, desiredLength, "%003x%003x%003x", (*color).red, (*color).green, (*color).blue);
    } else {
      snprintf(charArray, desiredLength, "%4d, %4d, %4d", (*color).red, (*color).green, (*color).blue);
    }
  }
}
