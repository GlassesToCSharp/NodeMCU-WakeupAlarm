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
