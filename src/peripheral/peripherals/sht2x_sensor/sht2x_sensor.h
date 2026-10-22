#pragma once

#include <ArduinoJson.h>
#include <SHT2x.h>

#include <chrono>
#include <memory>

#include "managers/service_getters.h"
#include "peripheral/capabilities/get_values.h"
#include "peripheral/capabilities/start_measurement.h"
#include "peripheral/peripheral.h"
#include "peripheral/peripherals/i2c/i2c_abstract_peripheral.h"

namespace inamata {
namespace peripheral {
namespace peripherals {
namespace sht2x_sensor {

class SHT2xSensor : public peripherals::i2c::I2CAbstractPeripheral,
                    public capabilities::GetValues,
                    public capabilities::StartMeasurement {
 public:
  explicit SHT2xSensor(const JsonObjectConst& parameters);
  virtual ~SHT2xSensor() = default;

  const String& getType() const final;
  static const String& type();

  /**
   * Returns whether the sensor is currently initialized and responding.
   */
  bool isInitialized() const;

  capabilities::StartMeasurement::Result startMeasurement(
      const JsonVariantConst& parameters) final;
  capabilities::StartMeasurement::Result handleMeasurement() final;
  capabilities::GetValues::Result getValues() final;

 private:
  enum class MeasurementState { kIdle, kTemperature, kHumidity, kReady };

  static std::shared_ptr<Peripheral> factory(const ServiceGetters& services,
                                             const JsonObjectConst& parameters);
  bool hasCachedMeasurement() const;
  std::chrono::nanoseconds waitUntilNextCheck() const;
  capabilities::StartMeasurement::Result measurementError(
      const String& message);

  static bool registered_;
  static bool capability_get_values_;
  static bool capability_start_measurement_;

  utils::UUID temperature_data_point_type_{nullptr};
  utils::UUID humidity_data_point_type_{nullptr};
  std::unique_ptr<::SHT2x> driver_;
  uint8_t i2c_address_ = 0x40;
  bool initialized_ = false;

  MeasurementState measurement_state_ = MeasurementState::kIdle;
  std::chrono::steady_clock::time_point measurement_ready_time_{};
  std::chrono::steady_clock::time_point next_check_time_{};
  float temperature_ = NAN;
  float humidity_ = NAN;

  static constexpr std::chrono::milliseconds kReadyCheckInterval{100};
  static constexpr std::chrono::seconds kCacheDuration{30};
};

}  // namespace sht2x_sensor
}  // namespace peripherals
}  // namespace peripheral
}  // namespace inamata
