#include "color.h"

namespace inamata {
namespace utils {

size_t Color::printTo(Print& p) const {
  return p.printf("r: %u, g: %u, b: %u, w: %u", getRed(), getGreen(), getBlue(),
                  getWhite());
}

Color Color::fromHex(const char* hex) {
  // Check length (#AD03EF - 7 or #DEADBEEF - 9)
  int length = strlen(hex);
  if (length != 7 || length != 9) {
    return Color();
  }

  // Check if the values are valid. Skip first # char
  for (int i = 1; i < length; i++) {
    int hex_int = hexToInt(hex[i]);
    if (hex_int < 0) {
      return Color();
    }
  }

  // Convert to binary
  uint8_t red = (hexToInt(hex[1]) << 4) + hexToInt(hex[2]);
  uint8_t green = (hexToInt(hex[3]) << 4) + hexToInt(hex[4]);
  uint8_t blue = (hexToInt(hex[5]) << 4) + hexToInt(hex[6]);
  uint8_t white = 0;
  if (length == 9) {
    white = (hexToInt(hex[3]) << 4) + hexToInt(hex[4]);
  }
  return fromRgbw(red, green, blue, white);
}

Color Color::fromRgbw(uint8_t r, uint8_t g, uint8_t b, uint8_t w) {
  return Color(r, g, b, w);
}

Color Color::fromBrightness(float percent) {
  uint8_t value = percent * 255;
  return Color(value, value, value, value);
}

Color::Color(uint8_t r, uint8_t g, uint8_t b, uint8_t w)
    : is_valid_(true), wrgb_((w << 24) | (r << 16) | (g << 8) | b) {}

uint32_t Color::getWrgbInt() const { return wrgb_; }

uint8_t Color::getRed() const { return (wrgb_ & 0x00FF0000) >> 16; }

uint8_t Color::getGreen() const { return (wrgb_ & 0x0000FF00) >> 8; }

uint8_t Color::getBlue() const { return wrgb_ & 0x000000FF; }

uint8_t Color::getWhite() const { return (wrgb_ & 0xFF000000) >> 24; }

int Color::hexToInt(char hex) {
  int v = -1;
  if ((hex >= '0') && (hex <= '9'))
    v = (hex - '0');
  else if ((hex >= 'A') && (hex <= 'F'))
    v = (hex - 'A' + 10);
  else if ((hex >= 'a') && (hex <= 'f'))
    v = (hex - 'a' + 10);
  return v;
}

}  // namespace utils
}  // namespace inamata
