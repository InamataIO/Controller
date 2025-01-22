#pragma once

#include <ArduinoJson.h>

#include "managers/service_getters.h"
#include "peripheral/capabilities/set_value.h"
#include "peripheral/peripheral.h"

namespace inamata {
namespace peripheral {
namespace peripherals {
namespace digital_out {

/**
 * Peripheral to control a GPIO output
 */
class DigitalOut : public Peripheral, public capabilities::SetValue {
 public:
  DigitalOut(const ServiceGetters& services, const JsonObjectConst& parameters);
  virtual ~DigitalOut() = default;

  // Type registration in the peripheral factory
  const String& getType() const final;
  static const String& type();

  /**
   * Turns the GPIO on or off
   *
   * \param value 1 sets the pin to its high state, 0 to its low state
   */
  void setValue(utils::ValueUnit value_unit) final;

  /**
   * Set an override value that preserves the setValue state
   *
   * \see clearOverride
   * \param state Logical on or off
   */
  void setOverride(bool state);

  /**
   * Clear the override value and restores the last setValue state
   */
  void clearOverride();

  /**
   * Returns the setValue state (ignore override)
   *
   * \return Logical on or off
   */
  bool getState();

 private:
  /**
   * Sets the output (digitalWrite) depending on state, override and active low
   */
  void setOutput();

  static std::shared_ptr<Peripheral> factory(const ServiceGetters& services,
                                             const JsonObjectConst& parameter);
  static bool registered_;
  static bool capability_set_value_;

  /// Interface to send data to the server
  std::shared_ptr<WebSocket> web_socket_;

  /// Main setValue state
  bool state_ = false;
  /// Logical override state, -1 if override cleared
  int8_t override_state_ = -1;

  /// The pin to be used as a GPIO output
  unsigned int pin_;

  // Check if an initial state should be set
  static const __FlashStringHelper* initial_state_key_;
  static const __FlashStringHelper* initial_state_key_error_;

  /// If active low logic is used. Inverts set value. False by default
  bool active_low_ = false;

  /// Data point type for the GPIO output pin state
  utils::UUID data_point_type_{nullptr};
};

}  // namespace digital_out
}  // namespace peripherals
}  // namespace peripheral
}  // namespace inamata
