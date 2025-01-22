#ifdef DEVICE_TYPE_TIAKI_CO2_MONITOR

#include "poll_remote_sensor.h"

#include "peripheral/fixed.h"

namespace inamata {
namespace tasks {
namespace fixed {

PollRemoteSensor::PollRemoteSensor(const ServiceGetters& services,
                                   Scheduler& scheduler,
                                   const JsonObjectConst& behavior_config)
    : PollAbstract(scheduler) {
  if (!isValid()) {
    return;
  }

  web_socket_ = services.getWebSocket();
  if (web_socket_ == nullptr) {
    setInvalid(services.web_socket_nullptr_error_);
    return;
  }

  measurement_wait_ = std::chrono::minutes(15);

  setIterations(TASK_FOREVER);
  enable();
}

const String& PollRemoteSensor::getType() const { return type(); }

const String& PollRemoteSensor::type() {
  static const String name{"PollRemoteSensor"};
  return name;
}

bool PollRemoteSensor::TaskCallback() {
  Task::delay(std::chrono::milliseconds(default_interval_).count());
  handlePoll();
  return true;
}

void PollRemoteSensor::handleResult(std::vector<utils::ValueUnit>& values) {
  JsonDocument doc_out;
  JsonObject result_object = doc_out.to<JsonObject>();
  web_socket_->packageTelemetry(values, modbus_input_->id, true, result_object);
  web_socket_->sendTelemetry(result_object);
}

const std::chrono::seconds PollRemoteSensor::default_interval_{60};

}  // namespace fixed
}  // namespace tasks
}  // namespace inamata

#endif