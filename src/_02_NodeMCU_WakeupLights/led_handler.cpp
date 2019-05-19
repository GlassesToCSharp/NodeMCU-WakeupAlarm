#include <Arduino.h>
#include <Ticker.h>  //Ticker Library

#include "led_handler.h"
#include "pinouts.h"

#define FLASH_TIME_PERIOD 2000 // 1000ms per flash cycle

Ticker ledHandler;

LedState currentState;
const int led = WIFI_LED;


void _ledIsr();
void _setLedTime(int milliseconds);
uint32_t _getLedTimeOn();
uint32_t _getLedTimeOff();


void initialiseLedHandler() {
  initialiseLedHandler(STATE_IDLE);
}

void initialiseLedHandler(LedState state) {
  pinMode(led, OUTPUT);
  pinMode(BOARD_LED, OUTPUT);
  setBoardLedState(true);

  setLedHandlerState(state);

  _ledIsr();
  Serial.println("LED Handler initialised.");
}

void _ledIsr() {
  // LEDs are active low on NodeMCU (?).
  if (!digitalRead(led)){
    // do the switching off
    _setLedTime(_getLedTimeOff());
  } else {
    // do the switching on
    _setLedTime(_getLedTimeOn());
  }
  digitalWrite(led, !(digitalRead(led)));
}

void setLedHandlerState(LedState newState) {
  currentState = newState;
  Serial.print("LED time on: ");
  Serial.println(_getLedTimeOn());
  Serial.print("LED time off: ");
  Serial.println(_getLedTimeOff());
}

void _setLedTime(int milliseconds) {
  if (ledHandler.active()) {
    ledHandler.detach();
  }

  ledHandler.once_ms(milliseconds, _ledIsr);
}

uint32_t _getLedTimeOn() {
  switch(currentState) {
    case STATE_FAILED:
    case STATE_IDLE:
    return (uint32_t)(FLASH_TIME_PERIOD / 2);

    case STATE_CONNECTING:
    return 200;

    case STATE_CONNECTED:
    return 50;
  }

  return 0;
}

uint32_t _getLedTimeOff() {
  return FLASH_TIME_PERIOD - _getLedTimeOn();
}

void setBoardLedState(bool enable) {
  digitalWrite(BOARD_LED, enable);
}
