#pragma once

#include <Arduino.h>
#include <TaskSchedulerDeclarations.h>

#include "tasks/base_task.h"

namespace inamata {
namespace tasks {
namespace factory_reset {

// Monitors the BOOT button independently of device configuration and services.
class FactoryReset : public BaseTask {
 public:
  explicit FactoryReset(Scheduler& scheduler);
  ~FactoryReset() override;

  const String& getType() const final;
  static const String& type();

  // Keep the setup-failure recovery window open until a press is processed.
  bool isPressInProgress() const;

 private:
  bool OnTaskEnable() final;
  bool TaskCallback() final;
  static void IRAM_ATTR onButtonChange(void* argument);

  static constexpr uint8_t button_pin_ = 0;
  static constexpr unsigned long hold_ms_ = 10000;
  static constexpr unsigned long idle_interval_ms_ = 500;
  static constexpr unsigned long pressed_interval_ms_ = 50;

  volatile uint32_t button_changes_ = 0;
  uint32_t observed_changes_ = 0;
  unsigned long press_start_ = 0;
  bool tracking_press_ = false;
  bool reset_armed_ = false;
};

}  // namespace factory_reset
}  // namespace tasks
}  // namespace inamata
