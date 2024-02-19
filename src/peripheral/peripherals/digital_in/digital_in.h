#pragma once

#include <ArduinoJson.h>

#include "managers/service_getters.h"
#include "peripheral/capabilities/get_values.h"
#include "peripheral/peripheral.h"

namespace inamata {
namespace peripheral {
namespace peripherals {
namespace digital_in {

/**
 * Peripheral to control a GPIO output
 */
class DigitalIn : public Peripheral, public capabilities::GetValues {
 public:
  DigitalIn(const JsonObjectConst& parameters);
  virtual ~DigitalIn() = default;

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

  /// If active low logic is used. Inverts set value. False by default
  bool active_low_ = false;

  /// The pin to be used as a GPIO output
  unsigned int pin_;

  /// Data point type for the GPIO output pin state
  utils::UUID data_point_type_{nullptr};

  /// How to setup the input GPIO
  static const __FlashStringHelper* input_type_key_;
  static const __FlashStringHelper* input_type_key_error_;
};

}  // namespace digital_in
}  // namespace peripherals
}  // namespace peripheral
}  // namespace inamata
