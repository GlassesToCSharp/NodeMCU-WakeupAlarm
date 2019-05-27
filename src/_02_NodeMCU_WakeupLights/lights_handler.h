typedef struct {
  uint16_t red;
  uint16_t green;
  uint16_t blue;
} LED_COLORS;

extern void initialiseLights();
extern void setLightColor(uint16_t red, uint16_t green, uint16_t blue);
extern void setLightColor(const LED_COLORS lights);
