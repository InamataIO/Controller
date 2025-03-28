#pragma once

#include <Arduino.h>

namespace inamata {

class Validator {
 public:
  static bool isValidName(const String &name);
  static bool isValidPhoneNumber(const String &phone);
  static bool isValidPlaceName(const String &place);
};

}  // namespace inamata