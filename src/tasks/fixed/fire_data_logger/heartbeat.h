#pragma once

#include "peripheral/fixed.h"
#include "peripheral/peripherals/digital_out/digital_out.h"
#include "tasks/base_task.h"

namespace inamata {
namespace tasks {
namespace fixed {

class Heartbeat : public BaseTask {
 public:
  using DigitalOut = peripheral::peripherals::digital_out::DigitalOut;

  Heartbeat(Scheduler& scheduler);
  virtual ~Heartbeat() = default;

  const String& getType() const final;
  static const String& type();

  bool TaskCallback();

 private:
  std::shared_ptr<DigitalOut> heartbeat_led_;
  /// Blinking LED state
  bool state = false;

  // Max time is ~72 minutes due to an overflow in the CPU load counter
  static const std::chrono::milliseconds default_interval_;
};

}  // namespace fixed
}  // namespace tasks
}  // namespace inamata