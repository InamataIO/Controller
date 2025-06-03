#pragma once

#include <ArduinoJson.h>
#include <PCA9539.h>

#include "managers/service_getters.h"
#include "peripheral/capabilities/get_values.h"
#include "peripheral/capabilities/set_value.h"
#include "peripheral/peripheral.h"
#include "peripheral/peripherals/i2c/i2c_abstract_peripheral.h"

namespace inamata {
namespace peripheral {
namespace peripherals {
namespace pca9539 {

class PCA9539 : public peripherals::i2c::I2CAbstractPeripheral,
                public capabilities::GetValues,
                public capabilities::SetValue {
 public:
  struct IO {
    IO(uint8_t pin, utils::UUID dpt) : pin(pin), dpt(dpt) {}

    uint8_t pin;
    utils::UUID dpt;
  };

  PCA9539(const JsonObjectConst& parameters);
  virtual ~PCA9539() = default;

  // Type registration in the peripheral factory
  const String& getType() const final;
  static const String& type();

  /**
   * Reads the VOC index
   *
   * \return A data point and its type
   */
  capabilities::GetValues::Result getValues() final;
  void setValue(utils::ValueUnit value_unit) final;

  /**
   * Gets the 16-bit input state
   *
   * \return The 16-bit input state
   */
  uint16_t getState();

 private:
  static std::shared_ptr<Peripheral> factory(const ServiceGetters& services,
                                             const JsonObjectConst& parameters);
  static bool registered_;
  static bool capability_get_values_;
  static bool capability_set_value_;

  static const uint8_t default_i2c_address_ = 0x74;
  uint8_t reset_pin_;
  ::PCA9539 driver_;

  std::vector<IO> inputs_;
  std::vector<IO> outputs_;
  // Invert input states
  bool active_low_in_ = false;
};

}  // namespace pca9539
}  // namespace peripherals
}  // namespace peripheral
}  // namespace inamata
