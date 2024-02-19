#pragma once

#include <ArduinoJson.h>

#include <memory>

#include "managers/service_getters.h"
#include "peripheral/capabilities/get_values.h"
#include "peripheral/peripheral.h"
#include "peripheral/peripherals/analog.h"

namespace inamata {
namespace peripheral {
namespace peripherals {
namespace analog_in {

/**
 * Peripheral to control a GPIO output
 */
class AnalogIn : public Peripheral,
                 public capabilities::GetValues,
                 private Analog {
 public:
  AnalogIn(const JsonObjectConst& parameters);
  virtual ~AnalogIn() = default;

  // Type registration in the peripheral factory
  const String& getType() const final;
  static const String& type();

  /**
   * Get the GPIO state
   *
   * \return The value 1 represents the high state, 0 its low state
   */
  capabilities::GetValues::Result getValues() final;

 private:
  static std::shared_ptr<Peripheral> factory(const ServiceGetters& services,
                                             const JsonObjectConst& parameter);
  static bool registered_;
  static bool capability_get_values_;

  /// The pin to be used as a GPIO output
  unsigned int pin_;
#ifdef ESP32
  static const std::array<uint8_t, 8> valid_pins_;
#elif ESP8266
  static const std::array<uint8_t, 1> valid_pins_;
#else
  static const std::array<uint8_t, 1> valid_pins_;
#endif
  static const __FlashStringHelper* pin_key_;
  static const __FlashStringHelper* pin_key_error_;
  static const __FlashStringHelper* invalid_pin_error_;
};

}  // namespace analog_in
}  // namespace peripherals
}  // namespace peripheral
}  // namespace inamata
