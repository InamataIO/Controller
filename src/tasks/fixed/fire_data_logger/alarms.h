#pragma once

#include "managers/services.h"
#include "peripheral/fixed.h"
#include "peripheral/peripherals/digital_in/digital_in.h"
#include "peripheral/peripherals/digital_out/digital_out.h"
#include "peripheral/peripherals/neo_pixel/neo_pixel.h"
#include "peripheral/peripherals/pca9536d/pca9536d.h"
#include "peripheral/peripherals/pca9539/pca9539.h"
#include "utils/limit_event.h"

namespace inamata {
namespace tasks {
namespace fixed {

class Alarms : public BaseTask {
 public:
  using DigitalOut = peripheral::peripherals::digital_out::DigitalOut;
  using DigitalIn = peripheral::peripherals::digital_in::DigitalIn;
  using NeoPixel = peripheral::peripherals::neo_pixel::NeoPixel;
  using PCA9539 = peripheral::peripherals::pca9539::PCA9539;
  using PCA9536D = peripheral::peripherals::pca9536d::PCA9536D;

  Alarms(const ServiceGetters& services, Scheduler& scheduler,
         const JsonObjectConst& behavior_config);
  virtual ~Alarms() = default;

  const String& getType() const final;
  static const String& type();

  bool TaskCallback();

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

  class ManualDebouncer : public Debouncer {
   public:
    void setCurrentState(bool state) { current_state_ = state; }

   private:
    bool readCurrentState() { return current_state_; }
    bool current_state_ = false;
  };

  void resetLimits();

  /**
   * Handle value from digital input
   *
   * \param values Measured floats and DPTs from IO expander and digital inputs
   */
  void handleResult(const utils::ValueUnit& value);

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

  void handleMaintenanceMode();

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
  std::shared_ptr<PCA9536D> input_bank_3_;
  std::array<std::shared_ptr<DigitalIn>, 8> input_bank_4_;

  // Limits
  // Digital alarms
  BoolLimit limit_diesel_1_fire_alarm_;
  BoolLimit limit_diesel_2_fire_alarm_;
  BoolLimit limit_diesel_3_fire_alarm_;
  BoolLimit limit_diesel_4_fire_alarm_;

  BoolLimit limit_diesel_1_pump_fail_;
  BoolLimit limit_diesel_2_pump_fail_;
  BoolLimit limit_diesel_3_pump_fail_;
  BoolLimit limit_diesel_4_pump_fail_;

  BoolLimit limit_diesel_1_battery_charger_fail_;
  BoolLimit limit_diesel_2_battery_charger_fail_;
  BoolLimit limit_diesel_3_battery_charger_fail_;
  BoolLimit limit_diesel_4_battery_charger_fail_;

  BoolLimit limit_diesel_1_low_oil_level_fail_;
  BoolLimit limit_diesel_2_low_oil_level_fail_;
  BoolLimit limit_diesel_3_low_oil_level_fail_;
  BoolLimit limit_diesel_4_low_oil_level_fail_;

  BoolLimit limit_diesel_control_circuit_fail_;
  BoolLimit limit_diesel_mains_fail_;
  BoolLimit limit_diesel_pump_fail_;
  BoolLimit limit_diesel_engine_overheat_fail_;
  BoolLimit limit_diesel_fuel_tank_low_;

  BoolLimit limit_electric_1_fire_alarm_;
  BoolLimit limit_electric_2_fire_alarm_;
  BoolLimit limit_electric_1_pump_fail_;
  BoolLimit limit_electric_2_pump_fail_;
  BoolLimit limit_electric_mains_fail_;
  BoolLimit limit_electric_control_circuit_fail_;

  BoolLimit limit_jockey_1_pump_fail_;
  BoolLimit limit_jockey_2_pump_fail_;
  BoolLimit limit_pumphouse_protection_alarm_;
  BoolLimit limit_annunciator_fault_;
  BoolLimit limit_pumphouse_flooding_alarm_;

  // Runtime alarms
  DurationLimit limit_duration_jockey_1_pump_run_;
  DurationLimit limit_duration_jockey_2_pump_run_;
  ActivationLimit limit_activation_jockey_1_pump_run_;
  ActivationLimit limit_activation_jockey_2_pump_run_;

  // Maintenance mode
  bool is_maintenace_mode = false;
  ManualDebouncer maintenance_button;
  utils::UUID maintenance_limit_id = nullptr;
  std::chrono::steady_clock::time_point last_maintenance_continue_event_sent =
      std::chrono::steady_clock::time_point::min();
  std::shared_ptr<peripheral::peripherals::neo_pixel::NeoPixel> status_led_;

  std::chrono::seconds continue_event_period_ = std::chrono::seconds(15);

  // Max time is ~72 minutes due to an overflow in the CPU load counter
  static const std::chrono::milliseconds default_interval_;
};

}  // namespace fixed
}  // namespace tasks
}  // namespace inamata