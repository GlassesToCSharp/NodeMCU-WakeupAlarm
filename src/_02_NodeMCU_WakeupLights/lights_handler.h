#ifndef LIGHTS_HANDLER_H_INCLUDED
#define LIGHTS_HANDLER_H_INCLUDED

// Reduced clutter on LED_COLORS struct. Helpful information:
// https://stackoverflow.com/a/14047520
struct LED_COLORS {
  float red;
  float green;
  float blue;

  LED_COLORS() : red(0), green(0), blue(0) {}
  LED_COLORS(float r, float g, float b) : red(r), green(g), blue(b) {}
  LED_COLORS(float value) : red(value), green(value), blue(value) {}

  bool operator==(const LED_COLORS& color) const {
    return red == color.red &&
      green == color.green &&
      blue == color.blue;
  }

  bool operator!=(const LED_COLORS& color) const {
    return red != color.red ||
      green != color.green ||
      blue != color.blue;
  }

  LED_COLORS& operator=(const LED_COLORS& color) {
    red = color.red;
    green = color.green;
    blue = color.blue;
    return *this;
  }

  LED_COLORS operator*(const double& value) const {
    LED_COLORS newColor;
    newColor.red = this->red * value;
    newColor.green = this->green * value;
    newColor.blue = this->blue * value;
    return newColor;
  }

  LED_COLORS operator+(const LED_COLORS& color) const {
    LED_COLORS newColor;
    newColor.red = this->red + color.red;
    newColor.green = this->green + color.green;
    newColor.blue = this->blue + color.blue;
    return newColor;
  }

  LED_COLORS operator-(const LED_COLORS& color) const {
    LED_COLORS newColor;
    newColor.red = this->red - color.red;
    newColor.green = this->green - color.green;
    newColor.blue = this->blue - color.blue;
    return newColor;
  }

#ifdef DEBUG
  void print(HardwareSerial& serial) const {
    serial.print(red);
    serial.print(", ");
    serial.print(green);
    serial.print(", ");
    serial.print(blue);
  }

  void println(HardwareSerial& serial) const {
    print(serial);
    serial.println();
  }
#endif

  int16_t redInt() const {
    return (int16_t)(this->red);
  }

  int16_t greenInt() const {
    return (int16_t)(this->green);
  }

  int16_t blueInt() const {
    return (int16_t)(this->blue);
  }
};

extern const LED_COLORS alarmColor;
extern const LED_COLORS lightsOff;
extern LED_COLORS currentLightColor;

extern const uint16_t fadeCapacity;
extern LED_COLORS fadeArray[];

extern void initialiseLights();
extern void setLightColor(int16_t red, int16_t green, int16_t blue);
extern void setLightColor(const LED_COLORS lights);
extern uint8_t to8bit(int16_t color);
extern uint16_t to10bit(uint8_t color);
extern void colorsToCharArray(char* charArray, const LED_COLORS* color, uint8_t desiredLength = 7, bool as8bitColors = true, bool asHex = true);

#endif
