
enum LedState {
  STATE_IDLE,
  STATE_CONNECTING,
  STATE_CONNECTED,
  STATE_FAILED
};



extern void initialiseLedHandler();
extern void initialiseLedHandler(LedState state);
extern void setLedHandlerState(LedState newState);
extern void setBoardLedState(bool enable);
