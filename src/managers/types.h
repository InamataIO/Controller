#pragma once

#include <Arduino.h>

#ifdef ENABLE_TRACE
#define TRACEF(format, ...)                                       \
  Serial.printf("%s::%s(%d) ", __FILE__, __FUNCTION__, __LINE__); \
  Serial.printf(String(F(format)).c_str(), __VA_ARGS__)
#define TRACELN(msg, ...)                                         \
  Serial.printf("%s::%s(%d) ", __FILE__, __FUNCTION__, __LINE__); \
  Serial.println(msg)
#else
#define TRACEF(format, ...) (void)sizeof(__VA_ARGS__)
#define TRACELN(format, ...)
#endif

namespace inamata {

class ErrorResult {
 public:
  /**
   * No error present
   */
  ErrorResult() {}

  /**
   * Specify where the error occured and any details pertaining to it
   *
   * \param who Where the error occured
   * \param detail Why the error occured
   */
  ErrorResult(String who, String detail) : who_(who), detail_(detail) {}

  /**
   * Checks if an error occured or not
   *
   * \return True if an error state exists
   */
  bool isError() const { return !who_.isEmpty() || !detail_.isEmpty(); }

  /**
   * Returns the who and detail in one string
   *
   * \return String with who and detail
   */
  String toString() { return who_ + F(": ") + detail_; }

  String who_;
  String detail_;
};

enum class Result {
  kSuccess = 0,             // Operation completed successfully
  kFailure = 1,             // Catch-all error state
  kNotReady = 2,            // Device not ready to be used
  kDeviceDisconnected = 3,  // Device could not be found
  kInvalidPin = 4,          // Invalid pin configuration
  kIdNotFound = 5,          // Sensor ID not found in respective category
  kNameAlreadyExists = 6,   // Device name already exists
};

struct WiFiAP {
  String ssid;
  String password;
  int16_t id;
  bool failed_connecting;
};

}  // namespace inamata