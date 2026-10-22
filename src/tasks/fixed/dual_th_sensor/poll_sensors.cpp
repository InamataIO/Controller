#ifdef DEVICE_TYPE_DUAL_TH_SENSOR

#include "poll_sensors.h"

#include <algorithm>

#include "peripheral/fixed.h"

namespace inamata {
namespace tasks {
namespace fixed {

PollSensors::PollSensors(const ServiceGetters& services, Scheduler& scheduler)
    : BaseTask(scheduler, Input(nullptr, true)) {
  if (!isValid()) {
    return;
  }

  web_socket_ = services.getWebSocket();
  if (web_socket_ == nullptr) {
    setInvalid(services.web_socket_nullptr_error_);
    return;
  }
  if (!setFixedPeripherals()) {
    return;
  }

  Task::setInterval(std::chrono::milliseconds(default_interval_).count());
  setIterations(TASK_FOREVER);
  enable();
}

const String& PollSensors::getType() const { return type(); }

const String& PollSensors::type() {
  static const String name{"PollSensors"};
  return name;
}

bool PollSensors::TaskCallback() {
  if (!web_socket_->isConnected()) {
    Task::delay(std::chrono::milliseconds(default_interval_).count());
    return true;
  }

  std::chrono::nanoseconds min_wait = std::chrono::nanoseconds::max();
  bool waiting = false;
  for (SensorInfo& sensor : sensors_) {
    if (sensor.cycle_complete) {
      continue;
    }

    auto measurement =
        sensor.measurement_active
            ? sensor.peripheral->handleMeasurement()
            : sensor.peripheral->startMeasurement(JsonVariantConst());
    if (measurement.error.isError()) {
      sensor.measurement_active = false;
      sensor.cycle_complete = true;
      continue;
    }
    if (measurement.wait != std::chrono::nanoseconds::zero()) {
      sensor.measurement_active = true;
      waiting = true;
      min_wait = std::min(min_wait, measurement.wait);
      continue;
    }

    sensor.measurement_active = false;
    sensor.cycle_complete = true;
    auto result = sensor.peripheral->getValues();
    if (result.error.isError() || result.values.empty()) {
      continue;
    }

    JsonDocument doc_out;
    JsonObject telemetry = doc_out.to<JsonObject>();
    WebSocket::packageTelemetry(result.values, sensor.peripheral->id, true,
                                telemetry);
    web_socket_->sendTelemetry(telemetry);
  }

  if (waiting) {
    const auto wait_ms =
        std::chrono::duration_cast<std::chrono::milliseconds>(min_wait).count();
    Task::delay(std::max<int64_t>(1, wait_ms));
  } else {
    for (SensorInfo& sensor : sensors_) {
      sensor.cycle_complete = false;
    }
    Task::delay(std::chrono::milliseconds(default_interval_).count());
  }
  return true;
}

bool PollSensors::setFixedPeripherals() {
  const char* peripheral_ids[] = {peripheral::fixed::peripheral_air_left_id,
                                  peripheral::fixed::peripheral_air_right_id};
  const String& expected_type =
      peripheral::peripherals::sht2x_sensor::SHT2xSensor::type();

  for (size_t i = 0; i < sensors_.size(); ++i) {
    const utils::UUID peripheral_id(peripheral_ids[i]);
    std::shared_ptr<peripheral::Peripheral> peripheral =
        Services::getPeripheralController().getPeripheral(peripheral_id);
    if (!peripheral) {
      setInvalid(
          peripheral::Peripheral::peripheralNotFoundError(peripheral_id));
      return false;
    }
    if (peripheral->getType() != expected_type || !peripheral->isValid()) {
      setInvalid(
          peripheral::Peripheral::notAValidError(peripheral_id, expected_type));
      return false;
    }
    sensors_[i].peripheral = std::static_pointer_cast<
        peripheral::peripherals::sht2x_sensor::SHT2xSensor>(peripheral);
  }
  return true;
}

const std::chrono::seconds PollSensors::default_interval_{60};

}  // namespace fixed
}  // namespace tasks
}  // namespace inamata

#endif
