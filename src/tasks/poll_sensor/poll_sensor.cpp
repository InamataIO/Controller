#include "poll_sensor.h"

#include "tasks/task_factory.h"

namespace inamata {
namespace tasks {
namespace poll_sensor {

PollSensor::PollSensor(const ServiceGetters& services,
                       const JsonObjectConst& parameters, Scheduler& scheduler)
    : GetValuesTask(parameters, scheduler) {
  if (!isValid()) {
    return;
  }

  web_socket_ = services.getWebSocket();
  if (web_socket_ == nullptr) {
    setInvalid(services.web_socket_nullptr_error_);
    return;
  }

  // Get the interval with which to poll the sensor
  JsonVariantConst interval_ms = parameters[interval_ms_key_];
  if (!interval_ms.is<float>()) {
    setInvalid(interval_ms_key_error_);
    return;
  }
  interval_ = std::chrono::milliseconds(interval_ms.as<int>());

  // Inifite iterations until end time
  Task::setIterations(-1);

  // Optionally get the duration for which to poll the sensor [default: forever]
  JsonVariantConst duration_ms = parameters[duration_ms_key_];
  if (duration_ms.is<float>()) {
    // If the duration is zero or negative, disable on first run
    if (duration_ms <= 0) {
      setInvalid();
      return;
    }
    run_until_ = std::chrono::steady_clock::now() +
                 std::chrono::milliseconds(duration_ms.as<int>());
  } else if (duration_ms.isNull()) {
    run_until_ = std::chrono::steady_clock::time_point::max();
  } else {
    setInvalid(duration_ms_key_error_);
    return;
  }

  // Check if the peripheral supports the startMeasurement capability. Start a
  // measurement if yes. Wait the returned amount of time to check the
  // measurement state. If doesn't support it, enable the task without delay.
  start_measurement_peripheral_ =
      std::dynamic_pointer_cast<peripheral::capabilities::StartMeasurement>(
          getPeripheral());
  if (start_measurement_peripheral_) {
    auto result = start_measurement_peripheral_->startMeasurement(parameters);
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

  // Create a JSON doc on the heap
  doc_out.clear();
  JsonObject result_object = doc_out.to<JsonObject>();

  // Read the peripheral's value units and its UUID and add them to the JSON doc
  ErrorResult error = packageValues(result_object);

  // Check if the values could be successfully read
  if (error.isError()) {
    setInvalid(error.toString());
    return false;
  }

  // Send the value units and peripheral UUID to the server
  web_socket_->sendTelemetry(getTaskID(), result_object);

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
  return new PollSensor(services, parameters, scheduler);
}

}  // namespace poll_sensor
}  // namespace tasks
}  // namespace inamata
