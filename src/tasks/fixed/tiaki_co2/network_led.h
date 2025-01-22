#pragma once

#include <TaskSchedulerDeclarations.h>

#include <array>
#include <chrono>

#include "leds.h"
#include "peripheral/peripherals/digital_out/digital_out.h"
#include "tasks/base_task.h"

namespace inamata {
namespace tasks {
namespace fixed {

class NetworkLed : public BaseTask {
 public:
  NetworkLed(const ServiceGetters& services, Scheduler& scheduler,
             const JsonObjectConst& behavior_config);
  virtual ~NetworkLed();

  using DigitalOut = peripheral::peripherals::digital_out::DigitalOut;

  const String& getType() const final;
  static const String& type();

  bool TaskCallback();

 private:
  enum class LedState {
    kNone,
    kWifiConnecting,
    kWebSocketConnecting,
    kWebSocketConnected,
    kImprov
  };

  void updateLedState();
  void updateBlink(const std::chrono::steady_clock::time_point now,
                   const std::chrono::milliseconds blink_interval);

  void wsMessageSent();

  std::shared_ptr<Network> network_;
  std::shared_ptr<WebSocket> web_socket_;
  std::shared_ptr<BleServer> ble_server_;

  LedState led_state_ = LedState::kNone;
  std::chrono::steady_clock::time_point last_blink_change_;
  uint8_t blink_pos_;
  const std::array<uint16_t, 2> wifi_connecting_blinks_ = {100, 1900};
  const std::array<uint16_t, 4> ws_connecting_blinks_ = {100, 100, 100, 1700};
  const std::array<uint16_t, 2> improv_blinks_ = {1000, 1000};

  bool activity_blink_active_ = false;

  utils::UUID led_dpt_;
  std::shared_ptr<DigitalOut> network_led_;
  std::chrono::steady_clock::time_point last_check_;
  const std::chrono::seconds check_interval_{1};

  // Max time is ~72 minutes due to an overflow in the CPU load counter
  static const std::chrono::milliseconds default_interval_;
};

}  // namespace fixed
}  // namespace tasks
}  // namespace inamata