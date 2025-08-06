#ifdef DEVICE_TYPE_TIAKI_CO2_MONITOR

#include "leds.h"

#include "managers/services.h"
#include "peripheral/fixed.h"

namespace inamata {
namespace tasks {
namespace fixed {

Leds::Leds() {
  auto& peripheral_controller = Services::getPeripheralController();
  led_fault_ =
      std::dynamic_pointer_cast<DigitalOut>(peripheral_controller.getPeripheral(
          utils::UUID(peripheral::fixed::peripheral_led_fault_id)));
  led_alarm_1_ =
      std::dynamic_pointer_cast<DigitalOut>(peripheral_controller.getPeripheral(
          utils::UUID(peripheral::fixed::peripheral_led_alarm_1_id)));
  led_alarm_2_ =
      std::dynamic_pointer_cast<DigitalOut>(peripheral_controller.getPeripheral(
          utils::UUID(peripheral::fixed::peripheral_led_alarm_2_id)));
  led_alarm_3_ =
      std::dynamic_pointer_cast<DigitalOut>(peripheral_controller.getPeripheral(
          utils::UUID(peripheral::fixed::peripheral_led_alarm_3_id)));
  led_alarm_4_ =
      std::dynamic_pointer_cast<DigitalOut>(peripheral_controller.getPeripheral(
          utils::UUID(peripheral::fixed::peripheral_led_alarm_4_id)));
  led_network_ =
      std::dynamic_pointer_cast<DigitalOut>(peripheral_controller.getPeripheral(
          utils::UUID(peripheral::fixed::peripheral_led_network_id)));
}

String Leds::checkLeds() {
  const char* peripheral_id = nullptr;
  if (!led_fault_) {
    peripheral_id = peripheral::fixed::peripheral_led_fault_id;
  } else if (!led_alarm_1_) {
    peripheral_id = peripheral::fixed::peripheral_led_alarm_1_id;
  } else if (!led_alarm_2_) {
    peripheral_id = peripheral::fixed::peripheral_led_alarm_2_id;
  } else if (!led_alarm_3_) {
    peripheral_id = peripheral::fixed::peripheral_led_alarm_3_id;
  } else if (!led_alarm_4_) {
    peripheral_id = peripheral::fixed::peripheral_led_alarm_4_id;
  } else if (!led_network_) {
    peripheral_id = peripheral::fixed::peripheral_led_network_id;
  }

  if (peripheral_id != nullptr) {
    return String(F("Loading LEDs: ")) + peripheral_id;
  }
  return String();
}

}  // namespace fixed
}  // namespace tasks
}  // namespace inamata

#endif