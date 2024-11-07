#pragma once

#include <Arduino.h>

namespace inamata {
namespace utils {

class LimitEvent {
 public:
  enum class Type {
    kStart,
    kContinue,
    kEnd,
  };

  static const __FlashStringHelper* event_type_key_;
  static const __FlashStringHelper* start_type_;
  static const __FlashStringHelper* continue_type_;
  static const __FlashStringHelper* end_type_;
};

}  // namespace utils
}  // namespace inamata