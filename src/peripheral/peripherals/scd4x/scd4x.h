#pragma once

#include <ArduinoJson.h>
#include <SensirionI2cScd4x.h>

#include "managers/service_getters.h"
#include "peripheral/capabilities/get_values.h"
#include "peripheral/capabilities/start_measurement.h"
#include "peripheral/peripheral.h"
#include "peripheral/peripherals/i2c/i2c_abstract_peripheral.h"

namespace inamata {
namespace peripheral {
namespace peripherals {
namespace scd4x {

/**
 * SCD41 operates in periodic measurement mode. The first measurement is
 * available five seconds after measurement starts, and the sensor refreshes
 * its CO2, temperature, and humidity values every five seconds thereafter.
 *
 * A completed measurement is cached for one sensor update interval. Multiple
 * tasks sharing this peripheral therefore receive the same reading
 * immediately, while only one task polls for the next sensor update.
 */
class SCD4X : public peripherals::i2c::I2CAbstractPeripheral,
              public capabilities::GetValues,
              public capabilities::StartMeasurement {
 public:
  SCD4X(const JsonObjectConst& parameters);
  virtual ~SCD4X() = default;

  // Type registration in the peripheral factory
  const String& getType() const final;
  static const String& type();

  capabilities::StartMeasurement::Result startMeasurement(
      const JsonVariantConst& parameters) final;
  capabilities::StartMeasurement::Result handleMeasurement() final;
  capabilities::GetValues::Result getValues() final;

 private:
  static std::shared_ptr<Peripheral> factory(const ServiceGetters& services,
                                             const JsonObjectConst& parameters);

  /**
   * Returns whether a completed measurement is still within the cache window.
   */
  bool hasCachedMeasurement() const;

  /**
   * Returns the remaining delay before another task may poll the sensor.
   *
   * Coordinates readiness checks across tasks that share this peripheral.
   */
  std::chrono::nanoseconds waitUntilNextMeasurementCheck() const;

  static bool registered_;
  static bool capability_get_values_;
  static bool capability_start_measurement_;

  utils::UUID co2_data_point_type_{nullptr};
  utils::UUID temperature_data_point_type_{nullptr};
  utils::UUID humidity_data_point_type_{nullptr};

  SensirionI2cScd4x driver_;
  uint8_t i2c_address_ = SCD40_I2C_ADDR_62;

  bool periodic_started_ = false;
  bool measurement_ready_ = false;
  std::chrono::steady_clock::time_point measurement_ready_time_{};
  std::chrono::steady_clock::time_point next_measurement_check_time_{};

  uint16_t last_co2_ppm_ = 0;
  float last_temperature_c_ = NAN;
  float last_humidity_rh_ = NAN;

  static constexpr std::chrono::milliseconds kPollInterval{250};
  static constexpr std::chrono::milliseconds kInitialMeasurementWait{5000};
  static constexpr std::chrono::milliseconds kCacheDuration{5000};

  static const __FlashStringHelper* start_measurement_error_;
  static const __FlashStringHelper* data_ready_status_error_;
  static const __FlashStringHelper* read_measurement_error_;
  static const __FlashStringHelper* co2_data_point_type_key_;
};

}  // namespace scd4x
}  // namespace peripherals
}  // namespace peripheral
}  // namespace inamata