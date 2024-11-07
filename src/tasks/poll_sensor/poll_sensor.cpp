#include "poll_sensor.h"

#include "tasks/task_factory.h"

namespace inamata {
namespace tasks {
namespace poll_sensor {

PollSensor::PollSensor(const ServiceGetters& services, Scheduler& scheduler,
                       const Input& input)
    : GetValuesTask(services, scheduler, input), interval_(input.interval) {
  if (!isValid()) {
    return;
  }

  web_socket_ = services.getWebSocket();
  if (web_socket_ == nullptr) {
    setInvalid(services.web_socket_nullptr_error_);
    return;
  }

  // Get the interval with which to poll the sensor
  if (interval_ <= std::chrono::milliseconds::zero()) {
    setInvalid(interval_ms_key_error_);
    return;
  }

  Task::setIterations(TASK_FOREVER);

  // Optionally get the duration for which to poll the sensor [default: forever]
  if (input.duration > std::chrono::milliseconds::zero()) {
    run_until_ = std::chrono::steady_clock::now() +
                 std::chrono::milliseconds(input.duration);
  } else {
    run_until_ = std::chrono::steady_clock::time_point::max();
  }

  // Check if the peripheral supports the startMeasurement capability. Start a
  // measurement if yes. Wait the returned amount of time to check the
  // measurement state. If doesn't support it, enable the task without delay.
  start_measurement_peripheral_ =
      std::dynamic_pointer_cast<peripheral::capabilities::StartMeasurement>(
          getPeripheral());
  if (start_measurement_peripheral_) {
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

const String& PollSensor::getType() const { return type(); }

const String& PollSensor::type() {
  static const String name{"PollSensor"};
  return name;
}

void PollSensor::populateInput(const JsonObjectConst& parameters,
                               Input& input) {
  GetValuesTask::populateInput(parameters, input);

  // Get the interval with which to poll the sensor
  JsonVariantConst interval_ms = parameters[interval_ms_key_];
  if (interval_ms.is<float>()) {
    input.interval = std::chrono::milliseconds(interval_ms.as<int64_t>());
  }

  JsonVariantConst duration_ms = parameters[duration_ms_key_];
  if (duration_ms.is<float>()) {
    input.duration = std::chrono::milliseconds(duration_ms.as<int64_t>());
  }
}

bool PollSensor::TaskCallback() {
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

  // Directly send it to the server
  sendTelemetry(result);

  // Check if to wait and run again or to end due to timeout
  if (run_until_ < std::chrono::steady_clock::now()) {
    return false;
  } else {
    Task::delay(std::chrono::duration_cast<std::chrono::milliseconds>(interval_)
                    .count());
    return true;
  }
}

bool PollSensor::registered_ = TaskFactory::registerTask(type(), factory);

BaseTask* PollSensor::factory(const ServiceGetters& services,
                              const JsonObjectConst& parameters,
                              Scheduler& scheduler) {
  Input input;
  populateInput(parameters, input);
  return new PollSensor(services, scheduler, input);
}

}  // namespace poll_sensor
}  // namespace tasks
}  // namespace inamata
