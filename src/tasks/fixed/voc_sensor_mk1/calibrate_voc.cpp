#ifdef DEVICE_TYPE_VOC_SENSOR_MK1

#include "calibrate_voc.h"

#include "managers/services.h"
#include "peripheral/capabilities/get_values.h"
#include "peripheral/fixed.h"

namespace inamata {
namespace tasks {
namespace fixed {

CalibrateVoc::CalibrateVoc(Scheduler& scheduler)
    : BaseTask(scheduler, Input(nullptr, true)), scheduler_(scheduler) {
  if (!isValid()) {
    return;
  }

  setIterations(TASK_FOREVER);
  Task::setInterval(std::chrono::milliseconds(default_interval_).count());

  bool success = setFixedPeripherals();
  if (!success) {
    return;
  }
}

const String& CalibrateVoc::getType() const { return type(); }

const String& CalibrateVoc::type() {
  static const String name{"CalibrateVoc"};
  return name;
}

bool CalibrateVoc::TaskCallback() {
  peripheral::capabilities::GetValues::Result result = air_sensor_->getValues();
  // Try again next iteration on error or missing reading
  if (result.error.isError() || result.values.size() < 1) {
    return true;
  }

  float humidity = 50;
  float temperature_c = 25;
  for (const auto& value_unit : result.values) {
    if (value_unit.data_point_type == humidity_dpt_) {
      humidity = value_unit.value;
    }
    if (value_unit.data_point_type == temperature_c_dpt_) {
      temperature_c = value_unit.value;
    }
  }

  voc_sensor_->startCalibration(humidity, temperature_c);
  return true;
}

bool CalibrateVoc::setFixedPeripherals() {
  utils::UUID peripheral_id(
      String(peripheral::fixed::peripheral_voc_id).c_str());
  std::shared_ptr<peripheral::Peripheral> peripheral =
      Services::getPeripheralController().getPeripheral(peripheral_id);
  if (!peripheral) {
    setInvalid(peripheral::Peripheral::peripheralNotFoundError(peripheral_id));
    return false;
  }

  // Since the UUID is specified externally, check the type
  const String& sgp40_type = peripheral::peripherals::sgp40::SGP40::type();
  if (peripheral->getType() == sgp40_type && peripheral->isValid()) {
    voc_sensor_ =
        std::static_pointer_cast<peripheral::peripherals::sgp40::SGP40>(
            peripheral);
  } else {
    setInvalid(
        peripheral::Peripheral::notAValidError(peripheral_id, sgp40_type));
    return false;
  }

  peripheral_id = String(peripheral::fixed::peripheral_air_id).c_str();
  peripheral = Services::getPeripheralController().getPeripheral(peripheral_id);
  if (!peripheral) {
    setInvalid(peripheral::Peripheral::peripheralNotFoundError(peripheral_id));
    return false;
  }

  // Since the UUID is specified externally, check the type
  const String& hdc2080_type =
      peripheral::peripherals::hdc2080::HDC2080::type();
  if (peripheral->getType() == hdc2080_type && peripheral->isValid()) {
    air_sensor_ =
        std::static_pointer_cast<peripheral::peripherals::hdc2080::HDC2080>(
            peripheral);
  } else {
    setInvalid(
        peripheral::Peripheral::notAValidError(peripheral_id, hdc2080_type));
    return false;
  }

  humidity_dpt_ = String(peripheral::fixed::dpt_humidity_id).c_str();
  temperature_c_dpt_ = String(peripheral::fixed::dpt_temperature_c_id).c_str();

  return true;
}

const std::chrono::minutes CalibrateVoc::default_interval_{5};

}  // namespace fixed
}  // namespace tasks
}  // namespace inamata

#endif