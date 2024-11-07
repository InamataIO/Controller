#pragma once

#include <TaskSchedulerDeclarations.h>

#include <chrono>

#include "managers/services.h"
#include "peripheral/peripherals/neo_pixel/neo_pixel.h"
#include "peripheral/peripherals/sgp40/sgp40.h"
#include "utils/limit_event.h"

namespace inamata {
namespace tasks {
namespace fixed {

class Alert : public BaseTask {
 public:
  Alert(const ServiceGetters& services, Scheduler& scheduler,
        const JsonObjectConst& behavior_config);
  virtual ~Alert() = default;

  const String& getType() const final;
  static const String& type();

  bool TaskCallback();

  void handleBehaviorConfig(const JsonObjectConst& config);

 private:
  /**
   * Set the VOC and RGB LED peripherals
   *
   * \return True on success
   */
  bool setFixedPeripherals();

  void handleLimit1(const std::chrono::steady_clock::time_point now,
                    const utils::ValueUnit& value_unit);
  void handleLimit2(const std::chrono::steady_clock::time_point now,
                    const utils::ValueUnit& value_unit);

  /**
   * Whether the limit being crossed should be ignored
   *
   * \return True if should be ignored
   */
  bool ignoreCrossedLimit(std::chrono::steady_clock::time_point& start,
                          const std::chrono::milliseconds duration,
                          const std::chrono::steady_clock::time_point now);
  void sendLimitEvent(const utils::UUID& limit_id,
                      const utils::ValueUnit& value_unit,
                      const utils::LimitEvent::Type type);

  Scheduler& scheduler_;
  ServiceGetters services_;
  std::shared_ptr<WebSocket> web_socket_;

  std::shared_ptr<peripheral::peripherals::neo_pixel::NeoPixel> status_led_;
  std::shared_ptr<peripheral::peripherals::sgp40::SGP40> voc_sensor_;

  utils::Color update_color_;
  const utils::Color danger_color_ = utils::Color::fromRgbw(255, 20, 20, 0);
  const utils::Color elevated_color_ = utils::Color::fromRgbw(255, 255, 20, 0);
  const utils::Color ok_color_ = utils::Color::fromRgbw(20, 255, 20, 0);

  /// Whether the limits have been crossed
  bool is_limit_1_crossed = false;
  bool is_limit_2_crossed = false;
  /// How long to wait before changing states (low-pass filter)
  std::chrono::seconds limit_1_delay_duration{30};
  std::chrono::seconds limit_2_delay_duration{15};
  std::chrono::steady_clock::time_point limit_1_delay_start =
      std::chrono::steady_clock::time_point::min();
  std::chrono::steady_clock::time_point limit_2_delay_start =
      std::chrono::steady_clock::time_point::min();

  /// Periodically send limit events if limit is crossed
  std::chrono::seconds continue_event_period_ = std::chrono::minutes(15);
  std::chrono::steady_clock::time_point last_continue_event_1_sent =
      std::chrono::steady_clock::time_point::min();
  std::chrono::steady_clock::time_point last_continue_event_2_sent =
      std::chrono::steady_clock::time_point::min();
  /// VOC index above which the alert is triggered
  float voc_alert_limit_1_ = 250;
  float voc_alert_limit_2_ = 400;
  /// Invalid ID means server has not configured it
  utils::UUID limit_1_id_{nullptr};
  utils::UUID limit_2_id_{nullptr};

  // Max time is ~72 minutes due to an overflow in the CPU load counter
  static const std::chrono::seconds default_interval_;
  static const __FlashStringHelper* limit_id_key_;
  static const __FlashStringHelper* fixed_peripheral_id_key_;
  static const __FlashStringHelper* fixed_dpt_id_key_;
};

}  // namespace fixed
}  // namespace tasks
}  // namespace inamata