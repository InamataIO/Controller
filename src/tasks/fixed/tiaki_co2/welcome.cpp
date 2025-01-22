#ifdef DEVICE_TYPE_TIAKI_CO2_MONITOR

#include "welcome.h"

#include "peripheral/fixed.h"

namespace inamata {
namespace tasks {
namespace fixed {

Welcome::Welcome(Scheduler& scheduler, const JsonObjectConst& behavior_config)
    : BaseTask(scheduler, Input(nullptr, true)),
      scheduler_(scheduler),
      peripheral_controller_(Services::getPeripheralController()),
      led_dpt_(utils::UUID::fromFSH(peripheral::fixed::dpt_led_id)),
      buzzer_dpt_(utils::UUID::fromFSH(peripheral::fixed::dpt_buzzer_id)) {
  if (!isValid()) {
    return;
  }
  String error = checkLeds();
  if (!error.isEmpty()) {
    setInvalid(error);
    return;
  }
  error = checkRelays();
  if (!error.isEmpty()) {
    setInvalid(error);
    return;
  }

  setIterations(TASK_FOREVER);
  enable();
}

const String& Welcome::getType() const { return type(); }

const String& Welcome::type() {
  static const String name{"Welcome"};
  return name;
}

bool Welcome::TaskCallback() {
  Task::delay(std::chrono::milliseconds(default_interval_).count());

  utils::ValueUnit led_on(1, led_dpt_);
  utils::ValueUnit led_off(0, led_dpt_);
  utils::ValueUnit buzzer_on(1, buzzer_dpt_);
  utils::ValueUnit buzzer_off(0, buzzer_dpt_);

  int active_led = 0;
  if (led_counter_ <= 5) {
    active_led = led_counter_;
  } else if (led_counter_ <= 12) {
    active_led = led_counter_ - 6;
  }

  buzzer_->setValue(active_led == 0 ? buzzer_on : buzzer_off);
  led_fault_->setValue(active_led == 0 ? led_on : led_off);
  led_alarm_1_->setValue(active_led == 1 ? led_on : led_off);
  led_alarm_2_->setValue(active_led == 2 ? led_on : led_off);
  led_alarm_3_->setValue(active_led == 3 ? led_on : led_off);
  led_alarm_4_->setValue(active_led == 4 ? led_on : led_off);
  led_network_->setValue(active_led == 5 ? led_on : led_off);

  // Iterate through the LEDs once
  led_counter_++;
  if (led_counter_ > 12) {
    return false;
  }
  return true;
}

const std::chrono::milliseconds Welcome::default_interval_{200};

}  // namespace fixed
}  // namespace tasks
}  // namespace inamata

#endif