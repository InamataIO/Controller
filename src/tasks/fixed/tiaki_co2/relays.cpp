#ifdef DEVICE_TYPE_TIAKI_CO2_MONITOR

#include "relays.h"

#include "managers/services.h"
#include "peripheral/fixed.h"

namespace inamata {
namespace tasks {
namespace fixed {

Relays::Relays() {
  auto& peripheral_controller = Services::getPeripheralController();

  buzzer_ =
      std::dynamic_pointer_cast<DigitalOut>(peripheral_controller.getPeripheral(
          utils::UUID::fromFSH(peripheral::fixed::peripheral_buzzer_id)));
  relay_alarm_1_ =
      std::dynamic_pointer_cast<DigitalOut>(peripheral_controller.getPeripheral(
          utils::UUID::fromFSH(peripheral::fixed::peripheral_relay_1_id)));
  relay_alarm_2_ =
      std::dynamic_pointer_cast<DigitalOut>(peripheral_controller.getPeripheral(
          utils::UUID::fromFSH(peripheral::fixed::peripheral_relay_2_id)));
  relay_alarm_3_ =
      std::dynamic_pointer_cast<DigitalOut>(peripheral_controller.getPeripheral(
          utils::UUID::fromFSH(peripheral::fixed::peripheral_relay_3_id)));
  relay_alarm_4_ =
      std::dynamic_pointer_cast<DigitalOut>(peripheral_controller.getPeripheral(
          utils::UUID::fromFSH(peripheral::fixed::peripheral_relay_4_id)));
  analog_out_ =
      std::dynamic_pointer_cast<GP8XXX>(peripheral_controller.getPeripheral(
          utils::UUID::fromFSH(peripheral::fixed::peripheral_analog_out_id)));
}

String Relays::checkRelays() {
  const __FlashStringHelper* peripheral_id = nullptr;
  if (!buzzer_) {
    peripheral_id = peripheral::fixed::peripheral_buzzer_id;
  } else if (!relay_alarm_1_) {
    peripheral_id = peripheral::fixed::peripheral_relay_1_id;
  } else if (!relay_alarm_2_) {
    peripheral_id = peripheral::fixed::peripheral_relay_2_id;
  } else if (!relay_alarm_3_) {
    peripheral_id = peripheral::fixed::peripheral_relay_3_id;
  } else if (!relay_alarm_4_) {
    peripheral_id = peripheral::fixed::peripheral_relay_4_id;
  } else if (!analog_out_) {
    peripheral_id = peripheral::fixed::peripheral_analog_out_id;
  }

  if (peripheral_id != nullptr) {
    return String(F("loading relays: ")) + peripheral_id;
  }
  return String();
}

}  // namespace fixed
}  // namespace tasks
}  // namespace inamata

#endif