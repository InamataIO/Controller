#pragma once

#include <ArduinoJson.h>
#include <HDC2080.h>

#include "managers/service_getters.h"
#include "peripheral/capabilities/get_values.h"
#include "peripheral/peripheral.h"
#include "peripheral/peripherals/i2c/i2c_abstract_peripheral.h"

namespace inamata {
namespace peripheral {
namespace peripherals {
namespace hdc2080 {

class HDC2080 : public peripherals::i2c::I2CAbstractPeripheral,
                public capabilities::GetValues {
 public:
  HDC2080(const JsonObjectConst& parameters);
  virtual ~HDC2080() = default;

  // Type registration in the peripheral factory
  const String& getType() const final;
  static const String& type();

  /**
   * Reads the temperature and humidity values
   *
   * \return The data points and their types
   */
  capabilities::GetValues::Result getValues() final;

 private:
  static std::shared_ptr<Peripheral> factory(const ServiceGetters& services,
                                             const JsonObjectConst& parameters);
  static bool registered_;
  static bool capability_get_values_;

  utils::UUID temperature_data_point_type_{nullptr};
  utils::UUID humidity_data_point_type_{nullptr};

  ::HDC2080 driver_;
  /// HDC2080 I2C address
  uint8_t i2c_address_;
};

}  // namespace hdc2080
}  // namespace peripherals
}  // namespace peripheral
}  // namespace inamata