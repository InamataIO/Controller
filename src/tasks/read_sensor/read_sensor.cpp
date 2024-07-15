#include "read_sensor.h"

#include "tasks/task_factory.h"

namespace inamata {
namespace tasks {
namespace read_sensor {

ReadSensor::ReadSensor(const ServiceGetters& services, Scheduler& scheduler,
                       const Input& input)
    : GetValuesTask(services, scheduler, input) {
  if (!isValid()) {
    return;
  }

  // Check if the peripheral supports the startMeasurement capability. Start a
  // measurement if yes. Wait the returned amount of time to check the
  // measurement state. If doesn't support it, enable the task without delay.
  start_measurement_peripheral_ =
      std::dynamic_pointer_cast<peripheral::capabilities::StartMeasurement>(
          getPeripheral());
  if (start_measurement_peripheral_) {
    // Repeatedly run task to call handleMeasurement
    Task::setIterations(-1);

    auto result = start_measurement_peripheral_->startMeasurement(
        input.start_measurement_parameters);
    if (result.error.isError()) {
      setInvalid(result.error.toString());
      return;
    }
    enableDelayed(
        std::chrono::duration_cast<std::chrono::milliseconds>(result.wait)
            .count());
  } else {
    enable();
  }
}

const String& ReadSensor::getType() const { return type(); }

const String& ReadSensor::type() {
  static const String name{"ReadSensor"};
  return name;
}

void ReadSensor::populateInput(const JsonObjectConst& parameters,
                               Input& input) {
  GetValuesTask::populateInput(parameters, input);

  JsonObjectConst start_measurement_parameters = parameters["smp"];
  if (!start_measurement_parameters.isNull()) {
    input.start_measurement_parameters = start_measurement_parameters;
  }
}

bool ReadSensor::TaskCallback() {
  // If using a startMeasurement peripheral, handle the measurement. Delay
  // reading values if the result includes a wait duration. Otherwise, read the
  // values and send them to the server.
  if (start_measurement_peripheral_) {
    auto result = start_measurement_peripheral_->handleMeasurement();
    if (result.error.isError()) {
      setInvalid(result.error.toString());
      return false;
    }
    if (result.wait.count() != 0) {
      Task::delay(
          std::chrono::duration_cast<std::chrono::milliseconds>(result.wait)
              .count());
      return true;
    }
  }

  // Get the values and check for error
  peripheral::capabilities::GetValues::Result result = peripheral_->getValues();
  if (result.error.isError()) {
    setInvalid(result.error.toString());
    return false;
  }

  // Handle the output externally (LAC) or directly send it to the server
  if (handle_output_) {
    handle_output_(result, *this);
  } else {
    sendTelemetry(result);
  }
  return false;
}

bool ReadSensor::registered_ = TaskFactory::registerTask(type(), factory);

BaseTask* ReadSensor::factory(const ServiceGetters& services,
                              const JsonObjectConst& parameters,
                              Scheduler& scheduler) {
  Input input;
  populateInput(parameters, input);
  return new ReadSensor(services, scheduler, input);
}

}  // namespace read_sensor
}  // namespace tasks
}  // namespace inamata
