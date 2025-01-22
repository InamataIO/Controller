#pragma once

#include <ArduinoJson.h>
#include <DFRobot_GP8XXX.h>

#include <memory>

#include "managers/service_getters.h"
#include "peripheral/capabilities/set_value.h"
#include "peripheral/peripheral.h"
#include "peripheral/peripherals/i2c/i2c_abstract_peripheral.h"

namespace inamata {
namespace peripheral {
namespace peripherals {
namespace gp8xxx {

class GP8XXX : public peripherals::i2c::I2CAbstractPeripheral,
               public capabilities::SetValue {
 public:
  enum class Variant { kGP8503, kGP8211S, kGP8512, kGP8413, kGP8302, kGP8403 };

  GP8XXX(const JsonObjectConst& parameters);
  virtual ~GP8XXX() = default;

  // Type registration in the peripheral factory
  const String& getType() const final;
  static const String& type();

  /**
   * Set the output current to the specified value
   *
   * \param value A value between 0.005 and 0.020 sets the current in amps
   */
  void setValue(utils::ValueUnit value_unit) final;

  /**
   * Set an override value that preserves the setValue value
   *
   * \see clearOverride
   * \param value Value for the output depending on variant
   */
  void setOverride(float value);

  /**
   * Clear the override value and restores the last setValue value
   */
  void clearOverride();

 private:
  /**
   * Sets the output depending on value, override and variant
   */
  void setOutput();

  static std::shared_ptr<Peripheral> factory(const ServiceGetters& services,
                                             const JsonObjectConst& parameter);
  static bool registered_;
  static bool capability_set_value_;

  /// Main setValue value
  float value_ = 0;
  /// Override value, NAN if override cleared
  float override_value_ = NAN;

  utils::UUID data_point_type_{nullptr};

  std::unique_ptr<DFRobot_GP8XXX_IIC> driver_;

  /// GP8XXX I2C address
  uint8_t i2c_address_ = -1;
  Variant variant_;
};

}  // namespace gp8xxx
}  // namespace peripherals
}  // namespace peripheral
}  // namespace inamata