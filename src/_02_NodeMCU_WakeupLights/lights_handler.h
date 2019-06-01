#ifndef PINOUTS_H_INCLUDED
#define PINOUTS_H_INCLUDED

typedef struct {
  uint16_t red;
  uint16_t green;
  uint16_t blue;
} LED_COLORS;

extern bool operator==(const LED_COLORS& first, const LED_COLORS& second);
extern bool operator!=(const LED_COLORS& first, const LED_COLORS& second);

extern void initialiseLights();
extern void setLightColor(uint16_t red, uint16_t green, uint16_t blue);
extern void setLightColor(const LED_COLORS lights);
extern uint8_t to8bit(uint16_t color);
extern uint16_t to10bit(uint8_t color);
extern void colorsToCharArray(char* charArray, const LED_COLORS* color, uint8_t desiredLength = 7, bool as8bitColors = true, bool asHex = true);

#endif
