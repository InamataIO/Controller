#pragma once

#include <ArduinoJson.h>

#include <chrono>
#include <memory>
#include <vector>

#include "managers/services.h"
#include "peripheral/capabilities/get_values.h"
#include "peripheral/capabilities/start_measurement.h"
#include "peripheral/peripheral.h"
#include "utils/limit_event.h"
#include "utils/uuid.h"
#include "utils/value_unit.h"

namespace inamata {
namespace tasks {
namespace fixed {

/**
 * Sends limit (threshold) events for dynamically registered multi-air-sensor
 * peripherals, configured entirely through the fixed config and behavior
 * config (no fixed compile-time peripheral/DPT IDs, no SMS/GSM).
 */
class Alarms : public BaseTask {
 public:
  using StartMeasurement = peripheral::capabilities::StartMeasurement;
  using GetValues = peripheral::capabilities::GetValues;

  Alarms(const ServiceGetters& services, Scheduler& scheduler,
         const JsonObjectConst& behavior_config);
  virtual ~Alarms() = default;

  const String& getType() const final;
  static const String& type();

  bool TaskCallback();

  void handleBehaviorConfig(const JsonObjectConst& config);

 private:
  /**
   * Threshold limit sent by the server for a single data point type
   */
  struct LimitInfo {
    enum class Condition { kGreaterThan, kLessThan };

    /// Behavior config key holding this limit's parameters
    String config_key;
    /// Invalid ID means the server has not configured it
    utils::UUID limit_id = nullptr;
    float threshold = NAN;
    /// Whether the limit triggers above or below the threshold
    Condition condition = Condition::kGreaterThan;
    bool is_limit_crossed = false;
    std::chrono::milliseconds delay_duration{0};
    std::chrono::steady_clock::time_point delay_start =
        std::chrono::steady_clock::time_point::min();
    std::chrono::steady_clock::time_point last_continue_event_sent =
        std::chrono::steady_clock::time_point::min();
  };

  /**
   * Groups the limits that apply to one data point type of a sensor
   */
  struct DptLimits {
    utils::UUID dpt;
    std::vector<LimitInfo> limits;
  };

  struct Sensor {
    enum class State {
      kIdle,
      kHandleMeasurement,
      kGetValues,
      kFailed,
    };

    std::shared_ptr<peripheral::Peripheral> peripheral;
    std::vector<DptLimits> dpt_limits;
    State state = State::kIdle;
    std::chrono::steady_clock::time_point wait_until =
        std::chrono::steady_clock::time_point::min();
  };

  void setLimitConfig(LimitInfo& limit_info, JsonVariantConst config);

  void handleLimit(const utils::ValueUnit& value_unit, LimitInfo& limit_info,
                   const utils::UUID& peripheral_id,
                   const std::chrono::steady_clock::time_point now);

  void sendLimitEvent(const LimitInfo& limit_info,
                      const utils::UUID& peripheral_id,
                      const utils::ValueUnit& value_unit,
                      const utils::LimitEvent::Type type);

  /**
   * Whether the limit being crossed should be ignored
   *
   * \return True if should be ignored
   */
  bool ignoreCrossedLimit(std::chrono::steady_clock::time_point& start,
                          const std::chrono::milliseconds duration,
                          const std::chrono::steady_clock::time_point now);

  std::vector<Sensor> sensors_;

  std::shared_ptr<WebSocket> web_socket_;

  std::chrono::steady_clock::time_point last_check_ =
      std::chrono::steady_clock::time_point::min();
  std::chrono::seconds check_interval_ = std::chrono::seconds(10);
  std::chrono::seconds continue_event_period_ = std::chrono::minutes(15);

  // Max time is ~72 minutes due to an overflow in the CPU load counter
  static const std::chrono::milliseconds default_interval_;
};

}  // namespace fixed
}  // namespace tasks
}  // namespace inamata
