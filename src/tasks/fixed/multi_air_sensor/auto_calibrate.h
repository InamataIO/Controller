#pragma once

#include <ArduinoJson.h>

#include <chrono>
#include <memory>
#include <vector>

#include "managers/service_getters.h"
#include "peripheral/capabilities/calibrate.h"
#include "peripheral/peripheral.h"
#include "tasks/base_task.h"
#include "utils/uuid.h"

namespace inamata {
namespace tasks {
namespace fixed {

/**
 * Periodically asks configured peripherals to check for and apply a
 * calibration candidate (e.g. an updated dust sensor baseline)
 *
 * Configured via the "AutoCalibrate" task entry in the device's fixed
 * config, listing the peripherals to be checked. This allows the task to be
 * reused for any peripheral that implements the Calibrate capability.
 */
class AutoCalibrate : public BaseTask {
 public:
  using Calibrate = peripheral::capabilities::Calibrate;

  AutoCalibrate(const ServiceGetters& services, Scheduler& scheduler,
                const JsonObjectConst& behavior_config);
  virtual ~AutoCalibrate() = default;

  const String& getType() const final;
  static const String& type();

  bool TaskCallback();

 private:
  struct Target {
    enum class State { kWaiting, kCalibrating };

    std::shared_ptr<Calibrate> peripheral;
    State state = State::kWaiting;
    std::chrono::steady_clock::time_point wait_until =
        std::chrono::steady_clock::time_point::min();
  };

  JsonObjectConst findConfig(JsonArrayConst tasks_config);

  std::vector<Target> targets_;

  /// How long to wait after a finished (or failed) calibration attempt
  /// before starting the next one. Configurable via check_interval_s
  std::chrono::seconds check_interval_{3600};

  static constexpr std::chrono::milliseconds default_interval_{1000};

  static const __FlashStringHelper* peripherals_key_;
  static const __FlashStringHelper* peripheral_uuid_key_;
  static const __FlashStringHelper* check_interval_s_key_;
};

}  // namespace fixed
}  // namespace tasks
}  // namespace inamata
