#pragma once

#include "leds.h"
#include "managers/services.h"
#include "peripheral/peripherals/digital_out/digital_out.h"
#include "peripheral/peripherals/gp8xxx/gp8xxx.h"
#include "peripheral/peripherals/modbus/modbus_client_input.h"
#include "poll_abstract.h"
#include "relays.h"
#include "utils/limit_event.h"

namespace inamata {
namespace tasks {
namespace fixed {

class Alarms : public PollAbstract, Leds, Relays {
 public:
  using DigitalOut = peripheral::peripherals::digital_out::DigitalOut;

  Alarms(const ServiceGetters& services, Scheduler& scheduler,
         const JsonObjectConst& behavior_config);
  virtual ~Alarms() = default;

  const String& getType() const final;
  static const String& type();

  bool TaskCallback();

  void handleBehaviorConfig(const JsonObjectConst& config);

 private:
  struct LimitInfo {
    float threshold;
    bool is_limit_crossed;
    /// Invalid ID means server has not configured it
    utils::UUID limit_id;
    std::chrono::steady_clock::time_point delay_start;
    std::chrono::steady_clock::time_point last_continue_event_sent;
    std::chrono::milliseconds delay_duration;
  };

  void setLimitConfig(LimitInfo& limit_info, JsonObjectConst config);

  /**
   * Handle values from remote sensor to trigger alarms
   *
   * \param values Measured floats and DPTs from remote sensor
   */
  void handleResult(std::vector<utils::ValueUnit>& values);

  void onError() final;

  void handleAlarms();
  void handleLimit(const utils::ValueUnit& value_unit, LimitInfo& limit_info,
                   const std::chrono::steady_clock::time_point now);
  void sendLimitEvent(const utils::UUID& limit_id,
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

  void setAlarmOutputs(LimitInfo& limit_info, std::shared_ptr<DigitalOut> led,
                       std::shared_ptr<DigitalOut> relay);

  void setFaultOutput(const bool is_fault);

  static LimitInfo makeLimitInfo(const float threshold,
                                 const std::chrono::seconds delay_duration);

  std::shared_ptr<WebSocket> web_socket_;

  float last_co2_ppm_ = NAN;
  float last_voc_index_ = NAN;

  utils::UUID led_dpt_;
  utils::UUID relay_dpt_;
  utils::UUID current_dpt_;
  utils::UUID buzzer_dpt_;
  utils::UUID co2_dpt_;
  utils::UUID voc_dpt_;

  // Limits
  /// CO2 ppm above which the alert is triggered
  LimitInfo co2_limit_1_ = makeLimitInfo(1000, std::chrono::seconds(30));
  LimitInfo co2_limit_2_ = makeLimitInfo(2000, std::chrono::seconds(30));
  LimitInfo co2_limit_3_ = makeLimitInfo(3500, std::chrono::seconds(30));
  LimitInfo co2_limit_4_ = makeLimitInfo(5000, std::chrono::seconds(10));
  LimitInfo voc_limit_1_ = makeLimitInfo(250, std::chrono::seconds(30));
  LimitInfo voc_limit_2_ = makeLimitInfo(400, std::chrono::seconds(30));

  std::chrono::seconds continue_event_period_ = std::chrono::minutes(15);

  // Max time is ~72 minutes due to an overflow in the CPU load counter
  static const std::chrono::milliseconds default_interval_;
};

}  // namespace fixed
}  // namespace tasks
}  // namespace inamata