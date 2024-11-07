#pragma once

#include <TaskSchedulerDeclarations.h>

#include <chrono>

#include "managers/services.h"
#include "peripheral/peripherals/sgp40/sgp40.h"

namespace inamata {
namespace tasks {
namespace fixed {

class PollVoc : public BaseTask {
 public:
  PollVoc(const ServiceGetters& services, Scheduler& scheduler,
          const JsonObjectConst& behavior_config);
  virtual ~PollVoc() = default;

  const String& getType() const final;
  static const String& type();

  bool TaskCallback();

 private:
  /**
   * Set the VOC and RGB LED peripherals
   *
   * \return True on success
   */
  bool setFixedPeripherals();

  Scheduler& scheduler_;
  ServiceGetters services_;
  std::shared_ptr<WebSocket> web_socket_;

  std::shared_ptr<peripheral::peripherals::sgp40::SGP40> voc_sensor_;

  // Max time is ~72 minutes due to an overflow in the CPU load counter
  static const std::chrono::seconds default_interval_;
};

}  // namespace fixed
}  // namespace tasks
}  // namespace inamata