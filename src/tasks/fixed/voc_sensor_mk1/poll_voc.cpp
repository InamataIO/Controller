#ifdef DEVICE_TYPE_VOC_SENSOR_MK1

#include "poll_voc.h"

#include "peripheral/fixed.h"

namespace inamata {
namespace tasks {
namespace fixed {

PollVoc::PollVoc(const ServiceGetters& services, Scheduler& scheduler,
                 const JsonObjectConst& behavior_config)
    : BaseTask(scheduler, Input(nullptr, true)),
      scheduler_(scheduler),
      services_(services) {
  if (!isValid()) {
    return;
  }

  web_socket_ = services.getWebSocket();
  if (web_socket_ == nullptr) {
    setInvalid(services.web_socket_nullptr_error_);
    return;
  }

  bool success = setFixedPeripherals();
  if (!success) {
    return;
  }

  Task::setInterval(std::chrono::milliseconds(default_interval_).count());
  setIterations(TASK_FOREVER);
  enable();
}

const String& PollVoc::getType() const { return type(); }

const String& PollVoc::type() {
  static const String name{"PollVoc"};
  return name;
}

bool PollVoc::TaskCallback() {
  if (!web_socket_->isConnected()) {
    return true;
  }

  // Get the values and check for error
  peripheral::capabilities::GetValues::Result result = voc_sensor_->getValues();
  // Try again next iteration on error or missing reading
  if (result.error.isError() || result.values.size() < 1) {
    return true;
  }

  JsonDocument doc_out;
  JsonObject telemetry = doc_out.to<JsonObject>();
  WebSocket::packageTelemetry(result.values, voc_sensor_->id, true, telemetry);
  web_socket_->sendTelemetry(telemetry);

  return true;
}

bool PollVoc::setFixedPeripherals() {
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
  return true;
}

const std::chrono::seconds PollVoc::default_interval_{10};

}  // namespace fixed
}  // namespace tasks
}  // namespace inamata

#endif