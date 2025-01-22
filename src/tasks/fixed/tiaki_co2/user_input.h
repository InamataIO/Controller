#pragma once

#include <Bounce2.h>
#include <TaskSchedulerDeclarations.h>

#include <chrono>

#include "leds.h"
#include "peripheral/peripherals/digital_in/digital_in.h"
#include "peripheral/peripherals/digital_out/digital_out.h"
#include "peripheral/peripherals/gp8xxx/gp8xxx.h"
#include "poll_abstract.h"
#include "relays.h"

namespace inamata {
namespace tasks {
namespace fixed {

class UserInput : public PollAbstract, Leds, Relays {
 public:
  using DigitalIn = peripheral::peripherals::digital_in::DigitalIn;
  using ModbusClientAdapter =
      peripheral::peripherals::modbus::ModbusClientAdapter;

  UserInput(const ServiceGetters& services, Scheduler& scheduler,
            const JsonObjectConst& behavior_config);
  virtual ~UserInput();

  const String& getType() const final;
  static const String& type();

  bool TaskCallback();

 private:
  void startBlinkMode();
  void abortBlinkMode();
  void handleBlinkMode();

  void startRelayTest();
  void abortRelayTest();
  void handleRelayTest();

  void clearOverrides();

  void identify();

  /**
   * Send modbus remote sensor results as telemetry
   *
   * \param values The floats and DPTs of the queried data
   */
  void handleResult(std::vector<utils::ValueUnit>& values);

  int relay_test_mode_ = 0;
  std::chrono::steady_clock::time_point last_relay_test_mode_change_;
  std::chrono::seconds relay_test_mode_duration_{3};

  std::shared_ptr<WebSocket> web_socket_;

  std::shared_ptr<DigitalIn> touch_1_;
  std::shared_ptr<DigitalIn> touch_2_;
  std::shared_ptr<ModbusClientAdapter> modbus_client_;

  uint8_t blink_mode_ = 0;
  bool blink_led_state_ = false;
  std::chrono::steady_clock::time_point last_blink_change_ =
      std::chrono::steady_clock::time_point::min();

  std::chrono::steady_clock::time_point identify_start_{};

  bool buzz_with_blink_ = false;
  uint8_t clear_buzzer_next_ = 0;

  // Max time is ~72 minutes due to an overflow in the CPU load counter
  static const std::chrono::milliseconds default_interval_;
};

}  // namespace fixed
}  // namespace tasks
}  // namespace inamata