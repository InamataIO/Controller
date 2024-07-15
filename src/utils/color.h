#pragma once

#include "Arduino.h"

namespace inamata {
namespace utils {

class Color : public Printable {
 public:
  /**
   * Create an empty invalid instance (#00000000)
   */
  Color() = default;
  virtual ~Color() = default;

  size_t printTo(Print& p) const;

  /*
   * From format #AD03EF (RGB) or #DEADBEEF (RGBW)
   */
  static Color fromHex(const char*);

  /**
   * From individual RGB (RGBW) components
   */
  static Color fromRgbw(uint8_t r, uint8_t g, uint8_t b, uint8_t w = 0);

  /**
   * From float 0 - 1 that maps to #00000000 to #FFFFFFFF
   */
  static Color fromBrightness(float percent);

  uint32_t getWrgbInt() const;

  uint8_t getRed() const;
  uint8_t getGreen() const;
  uint8_t getBlue() const;
  uint8_t getWhite() const;

  bool is_valid_ = false;

 private:
  Color(uint8_t r, uint8_t g, uint8_t b, uint8_t w);
  /**
   * Convert hex char ('0' - 'F', case-insensitve) to an int
   */
  static int hexToInt(char hex);

  uint32_t wrgb_;
};

}  // namespace utils

}  // namespace inamata
