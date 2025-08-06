#ifdef DEVICE_TYPE_TIAKI_CO2_MONITOR

#include "poll_abstract.h"

#include "managers/services.h"
#include "peripheral/fixed.h"

namespace inamata {
namespace tasks {
namespace fixed {

PollAbstract::PollAbstract(Scheduler& scheduler)
    : BaseTask(scheduler, Input(nullptr, true)) {
  if (!isValid()) {
    return;
  }

  const utils::UUID modbus_input_id =
      utils::UUID(peripheral::fixed::peripheral_modbus_sensor_in_id);
  modbus_input_ = std::dynamic_pointer_cast<ModbusClientInput>(
      Services::getPeripheralController().getPeripheral(modbus_input_id));
  if (!modbus_input_) {
    setInvalid(
        ErrorStore::genNotAValid(modbus_input_id, ModbusClientInput::type()));
    return;
  }

  setIterations(TASK_FOREVER);
  enable();
}

void PollAbstract::handlePoll() {
  const auto now = std::chrono::steady_clock::now();
  if (!is_measurement_running_ && last_measurement_ + measurement_wait_ < now) {
    startMeasurement(now);
  }
  if (is_measurement_running_) {
    handleMeasurement(now);
  }
}

void PollAbstract::startMeasurement(
    const std::chrono::steady_clock::time_point now) {
  auto result = modbus_input_->startMeasurement(JsonVariantConst());
  if (result.error.isError()) {
    errors_since_last_success_++;
    onError();
    markMeasurementDone(now, true);
    TRACEF("Start fail: %s - %d\r\n", result.error.toString().c_str(),
           errors_since_last_success_);
    return;
  }
  is_measurement_running_ = true;
  Task::delay(std::chrono::duration_cast<std::chrono::milliseconds>(result.wait)
                  .count());
}

void PollAbstract::handleMeasurement(
    const std::chrono::steady_clock::time_point now) {
  // Check if the results are ready and delay if not
  auto m_result = modbus_input_->handleMeasurement();
  if (m_result.error.isError()) {
    errors_since_last_success_++;
    onError();
    markMeasurementDone(now, true);
    TRACEF("Handle fail: %s - %u\r\n", m_result.error.toString().c_str(),
           errors_since_last_success_);
    return;
  }
  if (m_result.wait.count() != 0) {
    Task::delay(
        std::chrono::duration_cast<std::chrono::milliseconds>(m_result.wait)
            .count());
    return;
  }

  // Get the values and check for error
  auto v_result = modbus_input_->getValues();
  if (v_result.error.isError()) {
    errors_since_last_success_++;
    onError();
    markMeasurementDone(now, true);
    TRACEF("Get values fail: %s - %u\r\n", v_result.error.toString().c_str(),
           errors_since_last_success_);
    return;
  }

  markMeasurementDone(now);
  errors_since_last_success_ = 0;
  handleResult(v_result.values);
}

void PollAbstract::markMeasurementDone(
    const std::chrono::steady_clock::time_point now, const bool error) {
  is_measurement_running_ = false;
  if (!error) {
    last_measurement_ = now;
  } else {
    // Repeat measurement sooner (at 50% of usual time)
    last_measurement_ = now - (measurement_wait_ / 2);
  }
}

void PollAbstract::onError() {}

}  // namespace fixed
}  // namespace tasks
}  // namespace inamata

#endif