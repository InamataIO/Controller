#include "factory_reset.h"

#include "managers/storage.h"

namespace inamata {
namespace tasks {
namespace factory_reset {

FactoryReset::FactoryReset(Scheduler& scheduler)
    : BaseTask(scheduler, Input(nullptr, true)) {
  setIterations(TASK_FOREVER);
}

FactoryReset::~FactoryReset() { detachInterrupt(button_pin_); }

const String& FactoryReset::getType() const { return type(); }

const String& FactoryReset::type() {
  static const String name{"FactoryReset"};
  return name;
}

bool FactoryReset::OnTaskEnable() {
  pinMode(button_pin_, INPUT_PULLUP);
  attachInterruptArg(button_pin_, onButtonChange, this, CHANGE);
  return true;
}

void IRAM_ATTR FactoryReset::onButtonChange(void* argument) {
  auto* task = static_cast<FactoryReset*>(argument);
  task->button_changes_ = task->button_changes_ + 1;
}

bool FactoryReset::isPressInProgress() const {
  return tracking_press_ || digitalRead(button_pin_) == LOW ||
         button_changes_ != observed_changes_;
}

bool FactoryReset::TaskCallback() {
  const uint32_t changes = button_changes_;
  const bool pressed = digitalRead(button_pin_) == LOW;
  const unsigned long now = millis();

  if (reset_armed_) {
    if (!pressed) {
      TRACELN("BOOT button factory reset");
      Storage::recursiveRm("/");
      ESP.restart();
    }
  } else if (!pressed) {
    tracking_press_ = false;
  } else if (!tracking_press_ || changes != observed_changes_) {
    // Any transition, including a release between polls, restarts the hold.
    tracking_press_ = true;
    press_start_ = now;
  } else if (now - press_start_ >= hold_ms_) {
    // Wait for release so restarting does not enter the ROM bootloader.
    reset_armed_ = true;
    TRACELN("Factory reset armed; release BOOT to reset");
  }

  observed_changes_ = changes;
  Task::delay(tracking_press_ ? pressed_interval_ms_ : idle_interval_ms_);
  return true;
}

}  // namespace factory_reset
}  // namespace tasks
}  // namespace inamata
