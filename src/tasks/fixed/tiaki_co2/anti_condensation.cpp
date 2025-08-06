#ifdef DEVICE_TYPE_TIAKI_CO2_MONITOR

#include "anti_condensation.h"

#include "peripheral/fixed.h"

namespace inamata {
namespace tasks {
namespace fixed {

AntiCondensation::AntiCondensation(Scheduler& scheduler,
                                   const JsonObjectConst& behavior_config)
    : PollAbstract(scheduler) {
  if (!isValid()) {
    return;
  }

  const utils::UUID peripheral_id =
      utils::UUID(peripheral::fixed::peripheral_modbus_sensor_out_id);
  modbus_output_ = std::dynamic_pointer_cast<ModbusClientOutput>(
      Services::getPeripheralController().getPeripheral(peripheral_id));
  if (!modbus_output_) {
    setInvalid(
        ErrorStore::genNotAValid(peripheral_id, ModbusClientOutput::type()));
    return;
  }

  measurement_wait_ = std::chrono::seconds(15);

  setIterations(TASK_FOREVER);
  enable();
}

const String& AntiCondensation::getType() const { return type(); }

const String& AntiCondensation::type() {
  static const String name{"AntiCondensation"};
  return name;
}

bool AntiCondensation::TaskCallback() {
  Task::delay(std::chrono::milliseconds(default_interval_).count());
  handlePoll();
  return true;
}

void AntiCondensation::handleResult(std::vector<utils::ValueUnit>& values) {
  float humidity = NAN;
  const utils::UUID dpt_humidity =
      utils::UUID(peripheral::fixed::dpt_humidity_rh_id);
  for (const auto& value : values) {
    if (value.data_point_type == dpt_humidity) {
      humidity = value.value;
    }
  }
  if (isnan(humidity)) {
    TRACELN(F("No humidity"));
    return;
  }
  const bool heater_state = humidity > 65 ? 1 : 0;
  const utils::UUID heater_dpt =
      utils::UUID(peripheral::fixed::dpt_heater_id);
  modbus_output_->setValue(utils::ValueUnit(heater_state, heater_dpt));
}

const std::chrono::seconds AntiCondensation::default_interval_{1};

}  // namespace fixed
}  // namespace tasks
}  // namespace inamata

#endif