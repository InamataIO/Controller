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

  static const char* event_type_key_;
  static const char* start_type_;
  static const char* continue_type_;
  static const char* end_type_;
};

}  // namespace utils
}  // namespace inamata