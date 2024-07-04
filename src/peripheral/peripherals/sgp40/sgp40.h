#pragma once

#include <ArduinoJson.h>
#include <DFRobot_SGP40.h>

#include "managers/service_getters.h"
#include "peripheral/capabilities/calibrate.h"
#include "peripheral/capabilities/get_values.h"
#include "peripheral/peripheral.h"
#include "peripheral/peripherals/i2c/i2c_abstract_peripheral.h"

namespace inamata {
namespace peripheral {
namespace peripherals {
namespace sgp40 {

class SGP40 : public peripherals::i2c::I2CAbstractPeripheral,
              public capabilities::GetValues,
              public capabilities::Calibrate {
 public:
  SGP40(const JsonObjectConst& parameters);
  virtual ~SGP40() = default;

  // Type registration in the peripheral factory
  const String& getType() const final;
  static const String& type();

  /**
   * Reads the VOC index
   *
   * \return A data point and its type
   */
  capabilities::GetValues::Result getValues() final;

  /**
   * Calibrate the sensor with temperature and humidity data
   *
   * @param parameters Temperature (°C) and humidity %RH as a JSON dict
   */
  capabilities::Calibrate::Result startCalibration(
      const JsonObjectConst& parameters) final;

  /**
   * Complete calibration callback
   */
  capabilities::Calibrate::Result handleCalibration() final;

 private:
  static std::shared_ptr<Peripheral> factory(const ServiceGetters& services,
                                             const JsonObjectConst& parameters);
  static bool registered_;
  static bool capability_get_values_;
  static bool capability_calibrate_;

  utils::UUID data_point_type_{nullptr};

  DFRobot_SGP40 driver_;

  static const __FlashStringHelper* chip_init_error;
};

}  // namespace sgp40
}  // namespace peripherals
}  // namespace peripheral
}  // namespace inamata