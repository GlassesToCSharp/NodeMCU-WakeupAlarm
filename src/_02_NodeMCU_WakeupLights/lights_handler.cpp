#include <Arduino.h>

#include "lights_handler.h"
#include "pinouts.h"

#define RED_LIGHTS D1
#define GREEN_LIGHTS D2
#define BLUE_LIGHTS D5

const LED_COLORS alarmColor = { .red = 1023, .green = 1023, .blue = 0}; // In RGB format, 0-1023
const LED_COLORS lightsOff = {0, 0, 0}; // Lights off!
LED_COLORS currentLightColor = {0, 0, 0};

const uint16_t fadeDurationMs = 2000;
const uint16_t fadeCapacity = 1024;
const uint16_t fadeTransitionDelay = fadeDurationMs / fadeCapacity;
LED_COLORS fadeArray[fadeCapacity];

void interpolate(const LED_COLORS * firstValue, const LED_COLORS * secondValue, const int steps, LED_COLORS * dst);
void interpolateFade(const LED_COLORS * newColor, LED_COLORS * destination);

void initialiseLights() {
  pinMode(RED_LIGHTS, OUTPUT);
  pinMode(GREEN_LIGHTS, OUTPUT);
  pinMode(BLUE_LIGHTS, OUTPUT);
}

void setLightColor(int16_t red, int16_t green, int16_t blue) {
  analogWrite(RED_LIGHTS, 1023 - red);
  analogWrite(GREEN_LIGHTS, 1023 - green);
  analogWrite(BLUE_LIGHTS, 1023 - blue);
}

void setLightColor(const LED_COLORS lights) {
  interpolateFade(&lights, fadeArray);
  for(int i = 0; i < fadeCapacity; i++) {
    setLightColor((*(fadeArray + i)).redInt(), (*(fadeArray + i)).greenInt(), (*(fadeArray + i)).blueInt());
    // Add a delay to show the fade. Otherwise, it will fade too quickly.
    delay(fadeTransitionDelay);
    currentLightColor = *(fadeArray + i);
  }
}

uint8_t to8bit(int16_t color) {
  return map(color, 0, 1023, 0, 255);
}

uint16_t to10bit(uint8_t color) {
  return map(color, 0, 255, 0, 1023);
}

// Default parameters are defiend in the functino declaration.
// More information: https://stackoverflow.com/questions/2545720/error-default-argument-given-for-parameter-1
void colorsToCharArray(char* charArray, const LED_COLORS* color, uint8_t desiredLength /*= 7*/, bool as8bitColors /*= true*/, bool asHex /*= true*/) {
  if (as8bitColors) {
    if (asHex) {
      // Default option
      snprintf(charArray, desiredLength, "%02x%02x%02x", to8bit((*color).redInt()), to8bit((*color).greenInt()), to8bit((*color).blueInt()));
    } else {
      snprintf(charArray, desiredLength, "%3d, %3d, %3d", to8bit((*color).redInt()), to8bit((*color).greenInt()), to8bit((*color).blueInt()));
    }
  } else {
    if (asHex) {
      snprintf(charArray, desiredLength, "%003x%003x%003x", (*color).redInt(), (*color).greenInt(), (*color).blueInt());
    } else {
      snprintf(charArray, desiredLength, "%4d, %4d, %4d", (*color).redInt(), (*color).greenInt(), (*color).blueInt());
    }
  }
}

void interpolate(const LED_COLORS * firstValue, const LED_COLORS * secondValue, const int steps, LED_COLORS * dst) {
  double step = 1.0 / steps;
  LED_COLORS ledsStep = ((*secondValue) - (*firstValue)) * step;
  // first value in the destination is the first value in the source.
  *dst = *firstValue;
  for (uint16_t i = 1; i <= steps; i++) {
    dst++;
    LED_COLORS addedDifference = ledsStep * (double)i;
    *dst = (*firstValue) + addedDifference;
  }
}

void interpolateFade(const LED_COLORS * newColor, LED_COLORS * destination) {
  interpolate(&currentLightColor, newColor, fadeCapacity - 1, destination);
}
