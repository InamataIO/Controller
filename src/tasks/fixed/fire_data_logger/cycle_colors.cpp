#ifdef DEVICE_TYPE_FIRE_DATA_LOGGER

#include "cycle_colors.h"

#include "peripheral/fixed.h"

namespace inamata {
namespace tasks {
namespace fixed {

CycleColors::CycleColors(const ServiceGetters& services, Scheduler& scheduler,
                         const JsonObjectConst& behavior_config)
    : BaseTask(scheduler, Input(nullptr, true)),
      scheduler_(scheduler),
      services_(services) {
  if (!isValid()) {
    return;
  }

  bool success = setFixedPeripherals();
  if (!success) {
    return;
  }

  setIterations(TASK_FOREVER);
  enable();
}

const String& CycleColors::getType() const { return type(); }

const String& CycleColors::type() {
  static const String name{"CycleColors"};
  return name;
}

bool CycleColors::TaskCallback() {
  Task::delay(std::chrono::milliseconds(default_interval_).count());

  // Only use half brightness (256 is max)
  switch (iterations_ % 5) {
    case 0:
      status_led_->turnOn(utils::Color::fromRgbw(random(0, 128), random(0, 128),
                                                 random(0, 128)));
      break;
    case 1:
      status_led_->turnOff();
      break;
    case 2:
      status_led_->turnOn(utils::Color::fromRgbw(255, 0, 0, 0));
      break;
    case 3:
      status_led_->turnOn(utils::Color::fromRgbw(0, 255, 0, 0));
      break;
    case 4:
      status_led_->turnOn(utils::Color::fromRgbw(0, 0, 255, 0));
      break;
    default:
      break;
  }
  iterations_++;

  return true;
}

bool CycleColors::setFixedPeripherals() {
  utils::UUID peripheral_id(peripheral::fixed::peripheral_status_led_id);
  std::shared_ptr<peripheral::Peripheral> peripheral =
      Services::getPeripheralController().getPeripheral(peripheral_id);
  if (!peripheral) {
    setInvalid(peripheral::Peripheral::peripheralNotFoundError(peripheral_id));
    return false;
  }

  // Since the UUID is specified externally, check the type
  const String& neopixel_type =
      peripheral::peripherals::neo_pixel::NeoPixel::type();
  if (peripheral->getType() == neopixel_type && peripheral->isValid()) {
    status_led_ =
        std::static_pointer_cast<peripheral::peripherals::neo_pixel::NeoPixel>(
            peripheral);
  } else {
    setInvalid(
        peripheral::Peripheral::notAValidError(peripheral_id, neopixel_type));
    return false;
  }
  return true;
}

const std::chrono::seconds CycleColors::default_interval_{2};

}  // namespace fixed
}  // namespace tasks
}  // namespace inamata

#endif