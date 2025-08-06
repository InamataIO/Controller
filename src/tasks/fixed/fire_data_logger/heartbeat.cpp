#ifdef DEVICE_TYPE_FIRE_DATA_LOGGER

#include "heartbeat.h"

#include "managers/services.h"

namespace inamata {
namespace tasks {
namespace fixed {

Heartbeat::Heartbeat(Scheduler& scheduler)
    : BaseTask(scheduler, Input(nullptr, true)) {
  if (!isValid()) {
    return;
  }

  auto& peripheral_controller = Services::getPeripheralController();
  heartbeat_led_ =
      std::dynamic_pointer_cast<DigitalOut>(peripheral_controller.getPeripheral(
          peripheral::fixed::peripheral_heartbeat_led_id));
  if (!heartbeat_led_) {
    setInvalid("Missing peri");
    return;
  }

  setIterations(TASK_FOREVER);
  enable();
}
const String& Heartbeat::getType() const { return type(); }

const String& Heartbeat::type() {
  static const String name{"Heartbeat"};
  return name;
}

bool Heartbeat::TaskCallback() {
  Task::delay(std::chrono::milliseconds(default_interval_).count());

  state = !state;
  const utils::ValueUnit value_unit(state, peripheral::fixed::dpt_led_id);
  heartbeat_led_->setValue(value_unit);
  return true;
}

const std::chrono::milliseconds Heartbeat::default_interval_{500};

}  // namespace fixed
}  // namespace tasks
}  // namespace inamata
#endif