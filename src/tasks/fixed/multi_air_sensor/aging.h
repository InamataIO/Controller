#pragma once

#include <ArduinoJson.h>

#include <chrono>
#include <cstdint>
#include <vector>

#include "managers/services.h"
#include "utils/uuid.h"

namespace inamata {
namespace tasks {
namespace fixed {

/**
 * Tracks electrochemical sensor warm-up ("aging") time for multi-air-sensor
 * peripherals, configured entirely through the fixed config (no fixed
 * compile-time peripheral IDs).
 *
 * Other tasks (e.g. UpdateDisplay) can query the greatest remaining aging
 * time via the static getMaxHoursRemaining() without holding a reference to
 * this task.
 */
class Aging : public BaseTask {
 public:
  Aging(const ServiceGetters& services, Scheduler& scheduler,
        const JsonObjectConst& behavior_config);
  virtual ~Aging() = default;

  const String& getType() const final;
  static const String& type();

  bool TaskCallback();

  /// Greatest remaining aging hours across all tracked peripherals, 0 if none
  static uint16_t getMaxHoursRemaining();

  /// Remaining aging hours for a specific peripheral, 0 if none/untracked
  static uint16_t getHoursRemaining(const utils::UUID& uuid);

 private:
  struct AgingSensor {
    utils::UUID uuid;
    /// Configured total warm-up time
    uint16_t aging_hours_target;
    /// Elapsed powered-on hours, persisted in the calibration file; counts up
    /// and stops once it reaches aging_hours_target
    uint16_t aged_hours;
  };

  void updateAging(std::chrono::steady_clock::time_point now);

  static std::vector<AgingSensor> aging_sensors_;

  std::chrono::steady_clock::time_point last_save_ =
      std::chrono::steady_clock::time_point::min();
  static constexpr std::chrono::hours save_interval_{1};
  static constexpr std::chrono::minutes check_interval_{1};

  static uint16_t max_hours_remaining_;
};

}  // namespace fixed
}  // namespace tasks
}  // namespace inamata
