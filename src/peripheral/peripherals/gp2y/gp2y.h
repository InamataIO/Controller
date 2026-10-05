#pragma once

#include <ArduinoJson.h>

#include <chrono>
#include <memory>

#include "managers/service_getters.h"
#include "peripheral/capabilities/calibrate.h"
#include "peripheral/capabilities/get_values.h"
#include "peripheral/peripheral.h"

class GP2YDustSensor;

namespace inamata {
namespace peripheral {
namespace peripherals {
namespace gp2y {

class GP2Y : public Peripheral,
             public capabilities::GetValues,
             public capabilities::Calibrate {
 public:
  GP2Y(const JsonObjectConst& parameters);
  virtual ~GP2Y() = default;

  const String& getType() const final;
  static const String& type();

  capabilities::GetValues::Result getValues() final;

  capabilities::Calibrate::Result startCalibration(
      const JsonObjectConst& parameters) final;
  capabilities::Calibrate::Result handleCalibration() final;

 private:
  static std::shared_ptr<Peripheral> factory(const ServiceGetters& services,
                                             const JsonObjectConst& parameters);
  static bool registered_;
  static bool capability_get_values_;
  static bool capability_calibrate_;

  utils::UUID data_point_type_{nullptr};
  std::unique_ptr<GP2YDustSensor> sensor_;

  int led_pin;
  int analog_pin;

  /// Last measured dust density, served from cache within kCacheDuration_
  float last_density_ = NAN;
  std::chrono::steady_clock::time_point last_read_time_{};
  static constexpr std::chrono::milliseconds kCacheDuration_{1500};

  /// Number of getValues() reads since the last calibration candidate check.
  /// Mirrors GP2YDustSensor's internal minimum reading count so a candidate
  /// is likely ready by the time it is fetched
  uint16_t calibration_read_count_ = 0;
  static constexpr uint16_t kBaselineCandidateMinReadings = 10;

  static const __FlashStringHelper* led_pin_key_;
  static const __FlashStringHelper* led_pin_key_error_;
};

}  // namespace gp2y
}  // namespace peripherals
}  // namespace peripheral
}  // namespace inamata