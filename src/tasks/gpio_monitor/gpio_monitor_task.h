#pragma once

#include <TaskSchedulerDeclarations.h>

#include <chrono>

#include "managers/service_getters.h"
#include "tasks/base_task.h"

#define GPIO_MONITOR_RATE_MS 1000

namespace inamata {
namespace tasks {
namespace gpio_monitor {

class GPIO_MonitorTask : public BaseTask {
 public:
  GPIO_MonitorTask(const ServiceGetters& services, Scheduler& scheduler);
  virtual ~GPIO_MonitorTask();

  const String& getType() const final;
  static const String& type();

 private:
  uint16_t lastExpanderAState = 0xFFFF;
  uint16_t lastExpanderBState = 0xFFFF;

  bool OnTaskEnable() final;
  bool TaskCallback() final;

  void processExpanderDiff(uint16_t currentState, uint16_t& lastState,
                           uint8_t pinOffset);

  Scheduler& scheduler_;
  ServiceGetters services_;
};

}  // namespace gpio_monitor
}  // namespace tasks
}  // namespace inamata
