#pragma once

#include <TaskSchedulerDeclarations.h>

#include <array>
#include <chrono>
#include <cmath>

#include "managers/services.h"
#include "peripheral/peripherals/neo_pixel/neo_pixel.h"
#include "peripheral/peripherals/sht2x_sensor/sht2x_sensor.h"
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
  struct LimitInfo {
    enum class Condition { kLessThan, kGreaterThan };
    enum class Severity { kWarning, kDanger };

    String config_key;
    utils::UUID data_point_type;
    Condition condition;
    Severity severity;
    bool enabled = false;
    utils::UUID limit_id{nullptr};
    float threshold = NAN;
    bool crossed = false;
    float last_value = NAN;
    std::chrono::milliseconds delay{0};
    std::chrono::steady_clock::time_point delay_start =
        std::chrono::steady_clock::time_point::min();
    std::chrono::steady_clock::time_point last_continue =
        std::chrono::steady_clock::time_point::min();
  };

  struct SensorInfo {
    std::shared_ptr<peripheral::peripherals::sht2x_sensor::SHT2xSensor>
        peripheral;
    std::array<LimitInfo, 8> limits;
    bool measurement_active = false;
    bool cycle_complete = false;
  };

  static std::array<LimitInfo, 8> makeLimits(const char* side);
  bool setFixedPeripherals();
  void setLimitConfig(LimitInfo& limit, JsonVariantConst config,
                      const utils::UUID& peripheral_id);
  void handleLimit(LimitInfo& limit, const utils::ValueUnit& value,
                   const utils::UUID& peripheral_id,
                   std::chrono::steady_clock::time_point now);
  void sendLimitEvent(const LimitInfo& limit, const utils::ValueUnit& value,
                      const utils::UUID& peripheral_id,
                      utils::LimitEvent::Type event_type);
  std::chrono::milliseconds updateLed(
      std::chrono::steady_clock::time_point now);
  void showLimitStatus();

  std::array<SensorInfo, 2> sensors_;
  std::shared_ptr<peripheral::peripherals::neo_pixel::NeoPixel> status_led_;
  std::shared_ptr<WebSocket> web_socket_;

  bool missing_probe_blink_active_ = false;
  std::chrono::steady_clock::time_point missing_probe_blink_start_ =
      std::chrono::steady_clock::time_point::min();

  const utils::Color ok_color_ = utils::Color::fromRgbw(20, 255, 20);
  const utils::Color warning_color_ = utils::Color::fromRgbw(255, 255, 20);
  const utils::Color danger_color_ = utils::Color::fromRgbw(255, 20, 20);
  const std::chrono::seconds continue_event_period_{15 * 60};
  const std::chrono::seconds missing_probe_blink_period_{10};
  const std::chrono::milliseconds missing_probe_blink_duration_{50};
  static const std::chrono::seconds default_interval_;
};

}  // namespace fixed
}  // namespace tasks
}  // namespace inamata
