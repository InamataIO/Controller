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

  void handleBehaviorConfig(const JsonObjectConst& config);

 private:
  enum class SmsReminder { kNone, kInitialMessage, kReminder1, kReminder2 };

  struct BaseLimit {
    BaseLimit() = default;
    BaseLimit(const char* limit_name, const utils::UUID* fixed_peripheral_id)
        : limit_name(limit_name), fixed_peripheral_id(fixed_peripheral_id) {}
    virtual ~BaseLimit() = default;

    bool is_high = false;
    /// Invalid ID means server has not configured it
    utils::UUID limit_id = nullptr;
    const char* limit_name = "";
    SmsReminder sms_reminder = SmsReminder::kNone;
    const utils::UUID* fixed_peripheral_id = nullptr;
    std::chrono::steady_clock::time_point start_time =
        std::chrono::steady_clock::time_point::min();
    std::chrono::steady_clock::time_point last_continue_event_sent =
        std::chrono::steady_clock::time_point::min();
  };

  /**
   * Limit info for bool threshold limits
   */
  struct BoolLimit : public BaseLimit {
    BoolLimit() = default;
    BoolLimit(const char* limit_name, const utils::UUID* fixed_peripheral_id)
        : BaseLimit(limit_name, fixed_peripheral_id) {};

    std::chrono::steady_clock::time_point delay_start =
        std::chrono::steady_clock::time_point::min();
    std::chrono::milliseconds delay_duration = std::chrono::seconds(5);
  };

  /**
   * Limit info for activity based limits
   */
  struct DurationLimit : public BaseLimit {
    DurationLimit() = default;
    DurationLimit(const char* limit_name,
                  const utils::UUID* fixed_peripheral_id)
        : BaseLimit(limit_name, fixed_peripheral_id) {};

    /// How long it may be high before sending an alert
    std::chrono::seconds high_duration = std::chrono::seconds(15);
    /// When the last high started (if still active) for continuous high alert
    std::chrono::steady_clock::time_point high_start =
        std::chrono::steady_clock::time_point::min();
  };

  /**
   * Limit info for activations based limits
   */
  struct ActivationLimit : public BaseLimit {
    ActivationLimit() = default;
    ActivationLimit(const char* limit_name,
                    const utils::UUID* fixed_peripheral_id)
        : BaseLimit(limit_name, fixed_peripheral_id) {};

    /// Highs in the current period
    uint32_t highs = 0;
    /// High events allowed per period
    uint32_t highs_per_period = 10;
    /// The duration specifying the high period
    std::chrono::seconds period = std::chrono::hours(1);
    /// When the current period started
    std::chrono::steady_clock::time_point period_start =
        std::chrono::steady_clock::time_point::min();
    /// Whether over the limit in the current or last period
    bool is_over_limit = false;
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
                       const utils::ValueUnit& value_unit);
  void handleDurationLimit(DurationLimit& limit_info,
                           const utils::ValueUnit& value_unit);
  void handleActivationLimit(ActivationLimit& limit_info,
                             const utils::ValueUnit& value_unit);

  void sendLimitEvent(BaseLimit* limit, const utils::ValueUnit& value_unit,
                      const utils::LimitEvent::Type type);

  void sendStartStopSms(BaseLimit* limit, const utils::ValueUnit& value_unit,
                        const utils::LimitEvent::Type type);
  void handleSmsReminders();

  /**
   * Generate the text for an SMS alert
   *
   * \param limit The limit for which the alert will be sent
   * \param type Whether the alert started or ended
   * \return A GSM-7 encoding string to be sent to the GSM modem
   */
  String generateSmsAlertText(const BaseLimit* limit,
                              const utils::LimitEvent::Type type);

  /**
   * Generate the text for an SMS reminder
   *
   * \param limit The limit for which the reminder will be sent
   * \return A GSM-7 encoding string to be sent to the GSM modem
   */
  String generateSmsReminderText(const BaseLimit* limit);

  void sendMaintenanceDataPoint(bool on);

  /**
   * Whether the limit being crossed should be ignored
   *
   * \return True if should be ignored
   */
  bool ignoreCrossedLimit(std::chrono::steady_clock::time_point& start,
                          const std::chrono::milliseconds duration);

  void handleMaintenanceMode();

  static void setBoolLimitConfig(BoolLimit* limit, JsonObjectConst config);
  static void setDurationLimitConfig(DurationLimit* limit,
                                     JsonObjectConst config);
  static void setActivationLimitConfig(ActivationLimit* limit,
                                       JsonObjectConst config);
  static void setMaintenanceLimitConfig(BaseLimit* limit,
                                        JsonObjectConst config);

  static void resetBoolLimit(BoolLimit* limit);
  static void resetDurationLimit(DurationLimit* limit);
  static void resetActivationLimit(ActivationLimit* limit);

  std::shared_ptr<GsmNetwork> gsm_network_;
  std::shared_ptr<ConfigManager> config_manager_;
  // Testing safety to send only one SMS
  bool sent_sms_ = false;
  std::shared_ptr<WebSocket> web_socket_;

  std::shared_ptr<PCA9539> input_bank_1_;
  std::shared_ptr<PCA9539> input_bank_2_;
  std::shared_ptr<PCA9536D> input_bank_3_;
  std::array<std::shared_ptr<DigitalIn>, 9> input_bank_4_;

  std::shared_ptr<DigitalOut> relay_1_;
  std::shared_ptr<DigitalOut> relay_2_;

  // Limits
  // Digital alarms
  BoolLimit limit_diesel_1_fire_alarm_ = {
      "diesel_1_fire_alarm", &peripheral::fixed::peripheral_io_1_id};
  BoolLimit limit_diesel_2_fire_alarm_ = {
      "diesel_2_fire_alarm", &peripheral::fixed::peripheral_io_1_id};
  BoolLimit limit_diesel_3_fire_alarm_ = {
      "diesel_3_fire_alarm", &peripheral::fixed::peripheral_io_1_id};
  BoolLimit limit_diesel_4_fire_alarm_ = {
      "diesel_4_fire_alarm", &peripheral::fixed::peripheral_io_1_id};

  BoolLimit limit_diesel_1_pump_fail_ = {
      "diesel_1_pump_fail", &peripheral::fixed::peripheral_io_1_id};
  BoolLimit limit_diesel_2_pump_fail_ = {
      "diesel_2_pump_fail", &peripheral::fixed::peripheral_io_1_id};
  BoolLimit limit_diesel_3_pump_fail_ = {
      "diesel_3_pump_fail", &peripheral::fixed::peripheral_io_1_id};
  BoolLimit limit_diesel_4_pump_fail_ = {
      "diesel_4_pump_fail", &peripheral::fixed::peripheral_io_1_id};

  BoolLimit limit_diesel_1_battery_charger_fail_ = {
      "diesel_1_battery_charger_fail", &peripheral::fixed::peripheral_io_1_id};
  BoolLimit limit_diesel_2_battery_charger_fail_ = {
      "diesel_2_battery_charger_fail", &peripheral::fixed::peripheral_io_1_id};
  BoolLimit limit_diesel_3_battery_charger_fail_ = {
      "diesel_3_battery_charger_fail", &peripheral::fixed::peripheral_io_1_id};
  BoolLimit limit_diesel_4_battery_charger_fail_ = {
      "diesel_4_battery_charger_fail", &peripheral::fixed::peripheral_io_1_id};

  BoolLimit limit_diesel_1_low_oil_level_fail_ = {
      "diesel_1_low_oil_level_fail", &peripheral::fixed::peripheral_io_1_id};
  BoolLimit limit_diesel_2_low_oil_level_fail_ = {
      "diesel_2_low_oil_level_fail", &peripheral::fixed::peripheral_io_1_id};
  BoolLimit limit_diesel_3_low_oil_level_fail_ = {
      "diesel_3_low_oil_level_fail", &peripheral::fixed::peripheral_io_1_id};
  BoolLimit limit_diesel_4_low_oil_level_fail_ = {
      "diesel_4_low_oil_level_fail", &peripheral::fixed::peripheral_io_1_id};

  BoolLimit limit_diesel_control_circuit_fail_ = {
      "diesel_control_circuit_fail", &peripheral::fixed::peripheral_io_2_id};
  BoolLimit limit_diesel_mains_fail_ = {"diesel_mains_fail",
                                        &peripheral::fixed::peripheral_io_2_id};
  BoolLimit limit_diesel_pump_fail_ = {"diesel_pump_fail",
                                       &peripheral::fixed::peripheral_io_2_id};
  BoolLimit limit_diesel_engine_overheat_fail_ = {
      "diesel_engine_overheat_fail", &peripheral::fixed::peripheral_io_2_id};
  BoolLimit limit_diesel_fuel_tank_low_ = {
      "diesel_fuel_tank_low", &peripheral::fixed::peripheral_io_2_id};

  BoolLimit limit_electric_1_fire_alarm_ = {
      "electric_1_fire_alarm", &peripheral::fixed::peripheral_io_2_id};
  BoolLimit limit_electric_2_fire_alarm_ = {
      "electric_2_fire_alarm", &peripheral::fixed::peripheral_io_2_id};
  BoolLimit limit_electric_1_pump_fail_ = {
      "electric_1_pump_fail", &peripheral::fixed::peripheral_io_2_id};
  BoolLimit limit_electric_2_pump_fail_ = {
      "electric_2_pump_fail", &peripheral::fixed::peripheral_io_2_id};
  BoolLimit limit_electric_mains_fail_ = {
      "electric_mains_fail", &peripheral::fixed::peripheral_io_2_id};
  BoolLimit limit_electric_control_circuit_fail_ = {
      "electric_control_circuit_fail",
      &peripheral::fixed::peripheral_electric_control_circuit_fail_id};

  BoolLimit limit_jockey_1_pump_fail_ = {
      "jockey_1_pump_fail",
      &peripheral::fixed::peripheral_jockey_1_pump_fail_id};
  BoolLimit limit_jockey_2_pump_fail_ = {
      "jockey_2_pump_fail",
      &peripheral::fixed::peripheral_jockey_2_pump_fail_id};
  BoolLimit limit_pumphouse_protection_alarm_ = {
      "pumphouse_protection_alarm",
      &peripheral::fixed::peripheral_pumphouse_protection_alarm_id};
  BoolLimit limit_annunciator_fault_ = {
      "annunciator_fault", &peripheral::fixed::peripheral_annunciator_fault_id};
  BoolLimit limit_pumphouse_flooding_alarm_ = {
      "pumphouse_flooding_alarm",
      &peripheral::fixed::peripheral_pumphouse_flooding_alarm_id};
  BoolLimit limit_i41_ = {"i41", &peripheral::fixed::peripheral_i41_id};

  // Runtime alarms
  DurationLimit limit_duration_jockey_1_pump_run_ = {
      "duration_jockey_1_pump_run",
      &peripheral::fixed::peripheral_jockey_1_pump_run_id};
  DurationLimit limit_duration_jockey_2_pump_run_ = {
      "duration_jockey_2_pump_run",
      &peripheral::fixed::peripheral_jockey_2_pump_run_id};
  ActivationLimit limit_activation_jockey_1_pump_run_ = {
      "activation_jockey_1_pump_run",
      &peripheral::fixed::peripheral_jockey_1_pump_run_id};
  ActivationLimit limit_activation_jockey_2_pump_run_ = {
      "activation_jockey_2_pump_run",
      &peripheral::fixed::peripheral_jockey_2_pump_run_id};

  std::array<BoolLimit*, 33> bool_limits_ = {
      &limit_diesel_1_fire_alarm_,
      &limit_diesel_2_fire_alarm_,
      &limit_diesel_3_fire_alarm_,
      &limit_diesel_4_fire_alarm_,
      &limit_diesel_1_pump_fail_,
      &limit_diesel_2_pump_fail_,
      &limit_diesel_3_pump_fail_,
      &limit_diesel_4_pump_fail_,
      &limit_diesel_1_battery_charger_fail_,
      &limit_diesel_2_battery_charger_fail_,
      &limit_diesel_3_battery_charger_fail_,
      &limit_diesel_4_battery_charger_fail_,
      &limit_diesel_1_low_oil_level_fail_,
      &limit_diesel_2_low_oil_level_fail_,
      &limit_diesel_3_low_oil_level_fail_,
      &limit_diesel_4_low_oil_level_fail_,
      &limit_diesel_control_circuit_fail_,
      &limit_diesel_mains_fail_,
      &limit_diesel_pump_fail_,
      &limit_diesel_engine_overheat_fail_,
      &limit_diesel_fuel_tank_low_,
      &limit_electric_1_fire_alarm_,
      &limit_electric_2_fire_alarm_,
      &limit_electric_1_pump_fail_,
      &limit_electric_2_pump_fail_,
      &limit_electric_mains_fail_,
      &limit_electric_control_circuit_fail_,
      &limit_jockey_1_pump_fail_,
      &limit_jockey_2_pump_fail_,
      &limit_pumphouse_protection_alarm_,
      &limit_annunciator_fault_,
      &limit_pumphouse_flooding_alarm_,
      &limit_i41_,
  };
  /// Limits that should send reminders if not cleared by maintenance mode
  std::vector<BaseLimit*> reminder_limits_;

  // Maintenance mode
  bool is_maintenance_mode_ = false;
  ManualDebouncer maintenance_button_;
  BaseLimit maintenance_limit_;

  std::shared_ptr<peripheral::peripherals::neo_pixel::NeoPixel> status_led_;

  std::chrono::steady_clock::time_point now_;
  std::chrono::seconds continue_event_period_ = std::chrono::minutes(15);

  /// Max time is ~72 minutes due to an overflow in the CPU load counter
  static constexpr std::chrono::milliseconds default_interval_{500};
  /// Send reminder 1 this delay after event start
  static constexpr std::chrono::seconds reminder_1_delay_ =
      std::chrono::hours{1};
  /// Send reminder 2 this delay after event start
  static constexpr std::chrono::seconds reminder_2_delay_ =
      std::chrono::hours{2};
};

}  // namespace fixed
}  // namespace tasks
}  // namespace inamata