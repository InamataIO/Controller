#include "alert_sensor.h"

#include "tasks/task_factory.h"

namespace inamata {
namespace tasks {
namespace alert_sensor {

AlertSensor::AlertSensor(const ServiceGetters& services, Scheduler& scheduler,
                         const Input& input)
    : GetValuesTask(services, scheduler, input),
      handle_output_(std::bind(&AlertSensor::sendAlert, this, _1)),
      web_socket_(services.getWebSocket()),
      trigger_type_(input.trigger_type),
      threshold_(input.threshold),
      data_point_type_id_(input.data_point_type_id),
      trigger_count_limit_(input.trigger_count_limit) {
  if (!isValid()) {
    return;
  }

  if (web_socket_ == nullptr) {
    setInvalid(services.web_socket_nullptr_error_);
    return;
  }

  if (trigger_type_ == TriggerType::kInvalid) {
    setInvalid(trigger_type_key_error_);
    return;
  }

  if (isnan(threshold_)) {
    setInvalid(threshold_key_error_);
    return;
  }

  if (!data_point_type_id_.isValid()) {
    setInvalid(utils::ValueUnit::data_point_type_key_error);
    return;
  }

  if (input.interval <= std::chrono::milliseconds::zero()) {
    setInvalid(interval_ms_key_error_);
    return;
  }
  setInterval(std::chrono::milliseconds(input.interval).count());

  if (input.duration < std::chrono::milliseconds::zero()) {
    setIterations(TASK_FOREVER);
  } else {
    setIterations(std::chrono::milliseconds(input.duration).count() /
                  getInterval());
  }

  enable();
}

const String& AlertSensor::getType() const { return type(); }

const String& AlertSensor::type() {
  static const String name{"AlertSensor"};
  return name;
}

void AlertSensor::populateInput(const JsonObjectConst& parameters,
                                Input& input) {
  GetValuesTask::populateInput(parameters, input);

  JsonVariantConst trigger_type = parameters[trigger_type_key_];
  if (trigger_type.is<const char*>()) {
    input.trigger_type = toTriggerType(trigger_type.as<const char*>());
  }

  JsonVariantConst threshold = parameters[threshold_key_];
  if (threshold.is<float>()) {
    input.threshold = threshold;
  }

  JsonVariantConst data_point_type_id =
      parameters[utils::ValueUnit::data_point_type_key];
  if (!data_point_type_id.isNull()) {
    input.data_point_type_id = data_point_type_id;
  }

  JsonVariantConst interval_ms = parameters[interval_ms_key_];
  if (interval_ms.is<float>()) {
    input.interval = std::chrono::milliseconds(interval_ms.as<int64_t>());
  }

  JsonVariantConst duration_ms = parameters[duration_ms_key_];
  if (duration_ms.is<float>()) {
    input.duration = std::chrono::milliseconds(duration_ms.as<int64_t>());
  }

  JsonVariantConst trigger_count_limit = parameters[trigger_count_limit_key_];
  if (trigger_count_limit.is<float>()) {
    input.trigger_count_limit = trigger_count_limit.as<int32_t>();
  }
}

bool AlertSensor::TaskCallback() {
  auto result = getPeripheral()->getValues();
  if (result.error.isError()) {
    setInvalid(result.error.toString());
    return false;
  }

  // Find the specified data point type. Uses first found. Error if none found
  auto match_unit = [&](const utils::ValueUnit& value_unit) {
    return value_unit.data_point_type == data_point_type_id_;
  };
  const auto trigger_value_unit =
      std::find_if(result.values.cbegin(), result.values.cend(), match_unit);
  if (trigger_value_unit == result.values.end()) {
    setInvalid(String(F("Data point type not found: ")) +
               data_point_type_id_.toString());
    return false;
  }

  // Check the flank type if it should trigger
  if (isRisingThreshold(trigger_value_unit->value)) {
    if (trigger_type_ == TriggerType::kRising ||
        trigger_type_ == TriggerType::kEither) {
      handle_output_(TriggerType::kRising);
      incrementTriggerCount();
    }
  } else if (isFallingThreshold(trigger_value_unit->value)) {
    if (trigger_type_ == TriggerType::kFalling ||
        trigger_type_ == TriggerType::kEither) {
      handle_output_(TriggerType::kFalling);
      incrementTriggerCount();
    }
  }

  // Store the current value for the next iteration
  last_value_ = trigger_value_unit->value;
  return true;
}

AlertSensor::TriggerType AlertSensor::toTriggerType(const char* type) {
  for (uint8_t i = 0; i < trigger_type_name_.size(); i++) {
    if (String(trigger_type_name_[i]) == type) {
      return static_cast<TriggerType>(i + 1);
    }
  }
  return TriggerType::kInvalid;
}

const __FlashStringHelper* AlertSensor::fromTriggerType(
    const TriggerType trigger_type) {
  if (trigger_type != TriggerType::kInvalid) {
    return trigger_type_name_[static_cast<int>(trigger_type)];
  }
  return nullptr;
}

bool AlertSensor::sendAlert(TriggerType trigger_type) {
  if (trigger_type == TriggerType::kRising ||
      trigger_type == TriggerType::kFalling) {
    JsonDocument doc_out;
    doc_out[threshold_key_] = threshold_;

    const __FlashStringHelper* name = fromTriggerType(trigger_type);
    if (name != nullptr) {
      doc_out[trigger_type_key_] = name;
    } else {
      return false;
    }

    doc_out[WebSocket::telemetry_peripheral_key_] =
        getPeripheralUUID().toString();

    web_socket_->sendTelemetry(doc_out.to<JsonObject>(), &getTaskID());
    return true;
  }

  return false;
}

bool AlertSensor::isRisingThreshold(const float value) {
  return value > threshold_ && last_value_ <= threshold_;
}

bool AlertSensor::isFallingThreshold(const float value) {
  return value < threshold_ && last_value_ >= threshold_;
}

void AlertSensor::incrementTriggerCount() {
  if (trigger_count_limit_ > 0) {
    trigger_count_++;
    if (trigger_count_ >= trigger_count_limit_) {
      disable();
    }
  }
}

bool AlertSensor::registered_ = TaskFactory::registerTask(type(), factory);

BaseTask* AlertSensor::factory(const ServiceGetters& services,
                               const JsonObjectConst& parameters,
                               Scheduler& scheduler) {
  Input input;
  populateInput(parameters, input);
  return new AlertSensor(services, scheduler, input);
}

const std::array<const __FlashStringHelper*, 3>
    AlertSensor::trigger_type_name_ = {FPSTR("rising"), FPSTR("falling"),
                                       FPSTR("either")};

const __FlashStringHelper* AlertSensor::trigger_count_limit_key_ =
    FPSTR("trigger_count_limit");
const __FlashStringHelper* AlertSensor::trigger_count_limit_key_error_ =
    FPSTR("Missing property: trigger_count_limit (unsigned int)");

}  // namespace alert_sensor
}  // namespace tasks
}  // namespace inamata
