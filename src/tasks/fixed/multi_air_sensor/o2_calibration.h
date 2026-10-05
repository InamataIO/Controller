#pragma once

#include <ArduinoJson.h>

#include <chrono>
#include <memory>

#include "managers/services.h"
#include "peripheral/capabilities/get_values.h"
#include "peripheral/capabilities/start_measurement.h"
#include "peripheral/peripherals/lmp91000/lmp91000.hpp"
#include "utils/uuid.h"

namespace inamata {
namespace tasks {
namespace fixed {

/**
 * Performs a one-time reference calibration of an LMP91000 O2 sensor once
 * its configured aging period (see Aging) has elapsed, using the known
 * ambient O2 level as the reference. Samples are collected and averaged over
 * a fixed window before the new conversion factor is calculated, to smooth
 * out measurement noise. The resulting conversion factor is persisted via
 * Storage::storeCalibration so it survives reboots; if a factor is already
 * stored, it is applied immediately on construction and recalibration is
 * skipped.
 */
class O2Calibration : public BaseTask {
 public:
  using StartMeasurement = peripheral::capabilities::StartMeasurement;
  using GetValues = peripheral::capabilities::GetValues;
  using LMP91000 = peripheral::peripherals::lmp91000::LMP91000;

  O2Calibration(const ServiceGetters& services, Scheduler& scheduler,
                const JsonObjectConst& behavior_config);
  virtual ~O2Calibration() = default;

  const String& getType() const final;
  static const String& type();

  bool TaskCallback();

 private:
  enum class State {
    kWaitAging,
    kStartMeasurement,
    kHandleMeasurement,
    kGetValues,
    kDone,
  };

  void storeConversionFactor(float conversion_factor);

  utils::UUID uuid_{nullptr};
  std::shared_ptr<LMP91000> peripheral_;
  /// JSON-configured conversion factor, used as the pre-calibration baseline
  float configured_conversion_factor_ = 0;
  State state_ = State::kWaitAging;
  std::chrono::steady_clock::time_point wait_until_ =
      std::chrono::steady_clock::time_point::min();

  /// Sum/count of samples collected during the current averaging window
  float value_sum_ = 0;
  uint32_t sample_count_ = 0;
  std::chrono::steady_clock::time_point sampling_start_ =
      std::chrono::steady_clock::time_point::min();

  /// Known ambient O2 concentration (%), used as the calibration reference
  static constexpr float reference_o2_percent_ = 20.95f;
  static constexpr std::chrono::seconds aging_check_interval_{60};
  /// How long to collect and average samples before calibrating
  static constexpr std::chrono::seconds sampling_duration_{60};
  /// Gap between successive samples within the averaging window
  static constexpr std::chrono::seconds sample_interval_{2};
  static constexpr std::chrono::milliseconds default_interval_{250};
};

}  // namespace fixed
}  // namespace tasks
}  // namespace inamata
