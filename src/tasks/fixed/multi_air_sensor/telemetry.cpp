#ifdef DEVICE_TYPE_MULTI_AIR_SENSOR

#include "telemetry.h"

#include "utils/chrono.h"

namespace inamata {
namespace tasks {
namespace fixed {

Telemetry::Telemetry(const ServiceGetters& services, Scheduler& scheduler)
    : BaseTask(scheduler, Input(nullptr, true)),
      web_socket_(services.getWebSocket()) {
  if (!isValid()) {
    return;
  }

  if (web_socket_ == nullptr) {
    setInvalid(services.web_socket_nullptr_error_);
    return;
  }

  JsonDocument fixed_config_doc;
  Storage::loadFixedConfig(fixed_config_doc);

  JsonArrayConst peripherals_config =
      fixed_config_doc["peripherals"].as<JsonArrayConst>();
  auto& peripheral_controller = Services::getPeripheralController();
  String peripheral_error("Peripheral not found: ");

  for (JsonObjectConst peripheral_config : peripherals_config) {
    if (!peripheral_config["register"].as<bool>()) {
      continue;
    }

    auto peripheral =
        peripheral_controller.getPeripheral(peripheral_config["uuid"]);
    if (peripheral == nullptr) {
      // TODO: Serial startMeasurement/getValues vs parallel?
      setInvalid(peripheral_error +
                 peripheral_config["uuid"].as<const char*>());
      return;
    }
    sensors_.push_back({.peripheral = peripheral});
  }

  setIterations(TASK_FOREVER);
  enable();
}

const String& Telemetry::getType() const { return type(); }

const String& Telemetry::type() {
  static const String name{"Telemetry"};
  return name;
}

bool Telemetry::TaskCallback() {
  std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
  std::chrono::nanoseconds min_wait = std::chrono::nanoseconds::max();

  // Start new measurements for all sensors that support capability
  if (utils::hasTimedOut(last_send_, send_interval_, now)) {
    TRACEF("Start %lld, %lld, %lld\r\n", last_send_, send_interval_, now);
    last_send_ = now;

    for (Sensor& sensor : sensors_) {
      sensor.wait_until = now;
      auto measure_sensor =
          std::dynamic_pointer_cast<StartMeasurement>(sensor.peripheral);
      if (measure_sensor != nullptr) {
        StartMeasurement::Result result =
            measure_sensor->startMeasurement(JsonVariantConst());
        if (result.error.isError()) {
          TRACEF("Error startMeasurement: %s\r\n",
                 result.error.toString().c_str());
          sensor.state = Sensor::State::kIdle;
          continue;
        }
        sensor.state = Sensor::State::kHandleMeasurement;
        sensor.wait_until = now + result.wait;
        if (result.wait < min_wait) {
          min_wait = result.wait;
        }
      } else {
        sensor.state = Sensor::State::kGetValues;
      }
    }
  }

  bool sensor_updated = false;
  for (Sensor& sensor : sensors_) {
    if (sensor.state == Sensor::State::kHandleMeasurement &&
        sensor.wait_until <= now) {
      auto measure_sensor =
          std::dynamic_pointer_cast<StartMeasurement>(sensor.peripheral);
      if (measure_sensor != nullptr) {
        StartMeasurement::Result result = measure_sensor->handleMeasurement();
        if (result.error.isError()) {
          TRACEF("Error handleMeasurement: %s\r\n",
                 result.error.toString().c_str());
          sensor.state = Sensor::State::kFailed;
          continue;
        }
        if (result.wait == std::chrono::nanoseconds::zero()) {
          sensor.state = Sensor::State::kGetValues;
        }
        sensor.wait_until = now + result.wait;
        if (result.wait < min_wait) {
          min_wait = result.wait;
        }
      }
    }

    if (sensor.state == Sensor::State::kGetValues) {
      auto get_values_sensor =
          std::dynamic_pointer_cast<GetValues>(sensor.peripheral);
      if (get_values_sensor != nullptr) {
        GetValues::Result result = get_values_sensor->getValues();
        if (result.error.isError()) {
          TRACEF("Error getValues: %s\r\n", result.error.toString().c_str());
          sensor.state = Sensor::State::kFailed;
          continue;
        }
        sensor.values = result.values;
        sensor.state = Sensor::State::kUpdated;
        sensor_updated = true;
      }
    }
  }

  if (sensor_updated) {
    JsonDocument doc_out;
    for (Sensor& sensor : sensors_) {
      if (sensor.state == Sensor::State::kUpdated) {
        JsonObject result_object = doc_out.to<JsonObject>();
        WebSocket::packageTelemetry(sensor.values, sensor.peripheral->id, true,
                                    result_object);
        web_socket_->sendTelemetry(result_object);
        sensor.state = Sensor::State::kIdle;
      }
    }
  }

  if (min_wait == std::chrono::nanoseconds::max()) {
    Task::delay(std::chrono::milliseconds(default_interval_).count());
  } else {
    Task::delay(std::chrono::duration_cast<std::chrono::milliseconds>(min_wait)
                    .count());
  }
  return true;
}

const std::chrono::milliseconds Telemetry::default_interval_{1000};

}  // namespace fixed
}  // namespace tasks
}  // namespace inamata

#endif