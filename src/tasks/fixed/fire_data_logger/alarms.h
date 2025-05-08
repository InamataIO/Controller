#pragma once

#include "managers/services.h"
#include "peripheral/fixed.h"
#include "peripheral/peripherals/digital_in/digital_in.h"
#include "peripheral/peripherals/digital_out/digital_out.h"
#include "peripheral/peripherals/pca9539/pca9539.h"
#include "utils/limit_event.h"

namespace inamata {
namespace tasks {
namespace fixed {

class Alarms : public BaseTask {
 public:
  using DigitalOut = peripheral::peripherals::digital_out::DigitalOut;
  using DigitalIn = peripheral::peripherals::digital_in::DigitalIn;
  using PCA9539 = peripheral::peripherals::pca9539::PCA9539;

  Alarms(const ServiceGetters& services, Scheduler& scheduler,
         const JsonObjectConst& behavior_config);
  virtual ~Alarms() = default;

  const String& getType() const final;
  static const String& type();

  bool TaskCallback();

  void handleBehaviorConfig(const JsonObjectConst& config);

 private:
  /**
   * Limit info for bool threshold limits
   */
  struct BoolLimit {
    bool is_high;
    /// Invalid ID means server has not configured it
    utils::UUID limit_id;
    const utils::UUID* fixed_peripheral_id;
    std::chrono::steady_clock::time_point last_continue_event_sent;

    std::chrono::steady_clock::time_point delay_start;
    std::chrono::milliseconds delay_duration;
  };

  /**
   * Limit info for activity based limits
   */
  struct DurationLimit {
    bool is_high;
    /// Invalid ID means server has not configured it
    utils::UUID limit_id;
    const utils::UUID* fixed_peripheral_id;
    std::chrono::steady_clock::time_point last_continue_event_sent;

    /// How long it may be high before sending an alert
    std::chrono::seconds high_duration;
    /// When the last high started (if still active) for continuous high alert
    std::chrono::steady_clock::time_point high_start;
  };

  /**
   * Limit info for activations based limits
   */
  struct ActivationLimit {
    bool is_high;
    /// Invalid ID means server has not configured it
    utils::UUID limit_id;
    const utils::UUID* fixed_peripheral_id;
    std::chrono::steady_clock::time_point last_continue_event_sent;

    /// Highs in the current period
    uint32_t highs;
    /// High events allowed per period
    uint32_t highs_per_period;
    /// The duration specifying the high period
    std::chrono::seconds period;
    /// When the current period started
    std::chrono::steady_clock::time_point period_start;
    /// Whether over the limit in the current or last period
    bool is_over_limit;
  };

  /**
   * Handle value from digital input
   *
   * \param values Measured floats and DPTs from IO expander and digital inputs
   */
  void handleResult(const utils::ValueUnit& value);

  void handleAlarms();
  void handleBoolLimit(BoolLimit& limit_info,
                       const utils::ValueUnit& value_unit,
                       const std::chrono::steady_clock::time_point now);
  void handleDurationLimit(DurationLimit& limit_info,
                           const utils::ValueUnit& value_unit,
                           const std::chrono::steady_clock::time_point now);
  void handleActivationLimit(ActivationLimit& limit_info,
                             const utils::ValueUnit& value_unit,
                             const std::chrono::steady_clock::time_point now);

  void sendLimitEvent(const utils::UUID& limit_id,
                      const utils::UUID* fixed_peripheral_id,
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

  void setAlarmOutputs(BoolLimit& limit_info, std::shared_ptr<DigitalOut> led,
                       std::shared_ptr<DigitalOut> relay);

  void setFaultOutput(const bool is_fault);

  static BoolLimit makeBoolLimitInfo(
      const utils::UUID* fixed_peripheral_id,
      const std::chrono::seconds delay_duration = std::chrono::seconds(5));
  static DurationLimit makeDurationLimitInfo(
      const utils::UUID* fixed_peripheral_id,
      const std::chrono::seconds high_duration = std::chrono::seconds(15));
  static ActivationLimit makeActivationLimitInfo(
      const utils::UUID* fixed_peripheral_id,
      const uint32_t highs_per_period = 3,
      const std::chrono::seconds period = std::chrono::seconds(30));

  std::shared_ptr<WebSocket> web_socket_;

  std::shared_ptr<PCA9539> input_bank_1_;
  std::shared_ptr<PCA9539> input_bank_2_;
  std::array<std::shared_ptr<DigitalIn>, 8> input_bank_3_;
  std::shared_ptr<DigitalIn> maintenance_mode_;

  // Limits
  /// Digital alarms
  BoolLimit limit_diesel_1_fire_alarm_ =
      makeBoolLimitInfo(&peripheral::fixed::peripheral_io_1_id);
  BoolLimit limit_diesel_2_fire_alarm_ =
      makeBoolLimitInfo(&peripheral::fixed::peripheral_io_1_id);
  BoolLimit limit_diesel_3_fire_alarm_ =
      makeBoolLimitInfo(&peripheral::fixed::peripheral_io_1_id);
  BoolLimit limit_diesel_4_fire_alarm_ =
      makeBoolLimitInfo(&peripheral::fixed::peripheral_io_1_id);

  BoolLimit limit_diesel_1_pump_fail_ =
      makeBoolLimitInfo(&peripheral::fixed::peripheral_io_1_id);
  BoolLimit limit_diesel_2_pump_fail_ =
      makeBoolLimitInfo(&peripheral::fixed::peripheral_io_1_id);
  BoolLimit limit_diesel_3_pump_fail_ =
      makeBoolLimitInfo(&peripheral::fixed::peripheral_io_1_id);
  BoolLimit limit_diesel_4_pump_fail_ =
      makeBoolLimitInfo(&peripheral::fixed::peripheral_io_1_id);

  BoolLimit limit_diesel_1_battery_charger_fail_ =
      makeBoolLimitInfo(&peripheral::fixed::peripheral_io_1_id);
  BoolLimit limit_diesel_2_battery_charger_fail_ =
      makeBoolLimitInfo(&peripheral::fixed::peripheral_io_1_id);
  BoolLimit limit_diesel_3_battery_charger_fail_ =
      makeBoolLimitInfo(&peripheral::fixed::peripheral_io_1_id);
  BoolLimit limit_diesel_4_battery_charger_fail_ =
      makeBoolLimitInfo(&peripheral::fixed::peripheral_io_1_id);

  BoolLimit limit_diesel_1_low_oil_level_fail_ =
      makeBoolLimitInfo(&peripheral::fixed::peripheral_io_2_id);
  BoolLimit limit_diesel_2_low_oil_level_fail_ =
      makeBoolLimitInfo(&peripheral::fixed::peripheral_io_2_id);
  BoolLimit limit_diesel_3_low_oil_level_fail_ =
      makeBoolLimitInfo(&peripheral::fixed::peripheral_io_2_id);
  BoolLimit limit_diesel_4_low_oil_level_fail_ =
      makeBoolLimitInfo(&peripheral::fixed::peripheral_io_2_id);

  BoolLimit limit_diesel_control_circuit_fail_ =
      makeBoolLimitInfo(&peripheral::fixed::peripheral_io_2_id);
  BoolLimit limit_diesel_mains_fail_ =
      makeBoolLimitInfo(&peripheral::fixed::peripheral_io_2_id);
  BoolLimit limit_diesel_pump_fail_ =
      makeBoolLimitInfo(&peripheral::fixed::peripheral_io_2_id);
  BoolLimit limit_diesel_engine_overheat_fail_ =
      makeBoolLimitInfo(&peripheral::fixed::peripheral_io_2_id);
  BoolLimit limit_diesel_fuel_tank_low_ =
      makeBoolLimitInfo(&peripheral::fixed::peripheral_io_2_id);

  BoolLimit limit_electric_1_fire_alarm_ =
      makeBoolLimitInfo(&peripheral::fixed::peripheral_io_2_id);
  BoolLimit limit_electric_2_fire_alarm_ =
      makeBoolLimitInfo(&peripheral::fixed::peripheral_io_2_id);
  BoolLimit limit_electric_1_pump_fail_ =
      makeBoolLimitInfo(&peripheral::fixed::peripheral_io_2_id);
  BoolLimit limit_electric_2_pump_fail_ =
      makeBoolLimitInfo(&peripheral::fixed::peripheral_io_2_id);
  BoolLimit limit_electric_mains_fail_ =
      makeBoolLimitInfo(&peripheral::fixed::peripheral_io_2_id);
  BoolLimit limit_electric_control_circuit_fail_ = makeBoolLimitInfo(
      &peripheral::fixed::peripheral_electric_control_circuit_fail_id);

  BoolLimit limit_jockey_1_pump_fail_ =
      makeBoolLimitInfo(&peripheral::fixed::peripheral_jockey_1_pump_fail_id);
  BoolLimit limit_jockey_2_pump_fail_ =
      makeBoolLimitInfo(&peripheral::fixed::peripheral_jockey_2_pump_fail_id);
  BoolLimit limit_pumphouse_protection_alarm_ = makeBoolLimitInfo(
      &peripheral::fixed::peripheral_pumphouse_protection_alarm_id);
  BoolLimit limit_annunciator_fault_ =
      makeBoolLimitInfo(&peripheral::fixed::peripheral_annunciator_fault_id);
  BoolLimit limit_pumphouse_flooding_alarm_ = makeBoolLimitInfo(
      &peripheral::fixed::peripheral_pumphouse_flooding_alarm_id);

  /// Runtime alarms
  DurationLimit limit_duration_jockey_1_pump_run_ = makeDurationLimitInfo(
      &peripheral::fixed::peripheral_jockey_1_pump_run_id);
  DurationLimit limit_duration_jockey_2_pump_run_ = makeDurationLimitInfo(
      &peripheral::fixed::peripheral_jockey_2_pump_run_id);
  ActivationLimit limit_activation_jockey_1_pump_run_ = makeActivationLimitInfo(
      &peripheral::fixed::peripheral_jockey_1_pump_run_id);
  ActivationLimit limit_activation_jockey_2_pump_run_ = makeActivationLimitInfo(
      &peripheral::fixed::peripheral_jockey_2_pump_run_id);

  std::chrono::seconds continue_event_period_ = std::chrono::seconds(15);

  // Max time is ~72 minutes due to an overflow in the CPU load counter
  static const std::chrono::milliseconds default_interval_;
};

}  // namespace fixed
}  // namespace tasks
}  // namespace inamata