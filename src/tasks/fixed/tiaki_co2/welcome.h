#pragma once

#include <TaskSchedulerDeclarations.h>

#include <chrono>

#include "leds.h"
#include "managers/services.h"
#include "peripheral/peripherals/digital_out/digital_out.h"
#include "relays.h"
#include "tasks/base_task.h"

namespace inamata {
namespace tasks {
namespace fixed {

class Welcome : public BaseTask, Leds, Relays {
 public:
  using DigitalOut = peripheral::peripherals::digital_out::DigitalOut;

  Welcome(Scheduler& scheduler, const JsonObjectConst& behavior_config);
  virtual ~Welcome() = default;

  const String& getType() const final;
  static const String& type();

  bool TaskCallback();

 private:
  Scheduler& scheduler_;
  peripheral::PeripheralController& peripheral_controller_;

  utils::UUID led_dpt_;
  utils::UUID buzzer_dpt_;

  int led_counter_ = 0;

  // Max time is ~72 minutes due to an overflow in the CPU load counter
  static const std::chrono::milliseconds default_interval_;
};

}  // namespace fixed
}  // namespace tasks
}  // namespace inamata