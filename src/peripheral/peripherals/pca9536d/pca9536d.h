#pragma once

#include <ArduinoJson.h>
#include <PCA9536D.h>

#include "managers/service_getters.h"
#include "peripheral/capabilities/get_values.h"
#include "peripheral/capabilities/set_value.h"
#include "peripheral/peripheral.h"
#include "peripheral/peripherals/i2c/i2c_abstract_peripheral.h"

namespace inamata {
namespace peripheral {
namespace peripherals {
namespace pca9536d {

class PCA9536D : public peripherals::i2c::I2CAbstractPeripheral,
                 public capabilities::GetValues,
                 public capabilities::SetValue {
 public:
  struct IO {
    IO(uint8_t pin, utils::UUID dpt) : pin(pin), dpt(dpt) {}

    uint8_t pin;
    utils::UUID dpt;
  };

  PCA9536D(const JsonObjectConst& parameters);
  virtual ~PCA9536D() = default;

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

 private:
  static std::shared_ptr<Peripheral> factory(const ServiceGetters& services,
                                             const JsonObjectConst& parameters);
  static bool registered_;
  static bool capability_get_values_;
  static bool capability_set_value_;

  uint8_t reset_pin_;
  ::PCA9536 driver_;

  std::vector<IO> inputs_;
  std::vector<IO> outputs_;
  // Invert input states
  bool active_low_in_ = false;
};

}  // namespace pca9536d
}  // namespace peripherals
}  // namespace peripheral
}  // namespace inamata
