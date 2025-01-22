#include "set_value.h"

#include "managers/services.h"
#include "tasks/task_factory.h"

namespace inamata {
namespace tasks {
namespace set_value {

SetValue::SetValue(Scheduler& scheduler, const Input& input)
    : BaseTask(scheduler, input), value_unit_(input.value_unit) {
  // Abort if the base class failed initialization
  if (!isValid()) {
    return;
  }

  if (isnan(input.value_unit.value)) {
    setInvalid(utils::ValueUnit::value_key_error);
    return;
  }

  if (!input.value_unit.data_point_type.isValid()) {
    setInvalid(utils::ValueUnit::data_point_type_key_error);
    return;
  }

  peripheral_id_ = input.peripheral_id;
  if (!peripheral_id_.isValid()) {
    setInvalid(ErrorStore::genMissingProperty(
        WebSocket::telemetry_peripheral_key_, ErrorStore::KeyType::kUUID));
    return;
  }

  // Search for the peripheral for the given ID
  auto peripheral =
      Services::getPeripheralController().getPeripheral(peripheral_id_);
  if (!peripheral) {
    setInvalid(peripheral::Peripheral::peripheralNotFoundError(peripheral_id_));
    return;
  }

  // Check that the peripheral supports the SetValue interface capability
  peripheral_ =
      std::dynamic_pointer_cast<peripheral::capabilities::SetValue>(peripheral);
  if (!peripheral_) {
    setInvalid(peripheral::capabilities::SetValue::invalidTypeError(
        input.peripheral_id, peripheral));
    return;
  }
  enable();
}

const String& SetValue::getType() const { return type(); }

const String& SetValue::type() {
  static const String name{"SetValue"};
  return name;
}

void SetValue::populateInput(const JsonObjectConst& parameters, Input& input) {
  BaseTask::populateInput(parameters, input);

  JsonVariantConst peripheral_id =
      parameters[WebSocket::telemetry_peripheral_key_];
  if (!peripheral_id.isNull()) {
    input.peripheral_id = peripheral_id;
  }

  JsonVariantConst value = parameters[utils::ValueUnit::value_key];
  if (value.is<float>()) {
    input.value_unit.value = value;
  }

  JsonVariantConst data_point_type_id =
      parameters[utils::ValueUnit::data_point_type_key];
  if (!data_point_type_id.isNull()) {
    input.value_unit.data_point_type = data_point_type_id;
  }
}

bool SetValue::TaskCallback() {
  peripheral_->setValue(value_unit_);
  if (handle_output_) {
    handle_output_(value_unit_, *this);
  } else {
    if (send_data_point_) {
      sendTelemetry(value_unit_);
    }
  }
  return false;
}

void SetValue::sendTelemetry(utils::ValueUnit value_unit,
                             const utils::UUID* lac_id) {
  JsonDocument doc_out;
  JsonObject telemetry_doc = doc_out.to<JsonObject>();
  JsonArray value_units_doc =
      telemetry_doc[utils::ValueUnit::data_points_key].to<JsonArray>();
  JsonObject value_unit_object = value_units_doc.add<JsonObject>();
  value_unit_object[utils::ValueUnit::value_key] = value_unit_.value;
  value_unit_object[utils::ValueUnit::data_point_type_key] =
      value_unit_.data_point_type.toString();
  telemetry_doc[WebSocket::telemetry_peripheral_key_] =
      peripheral_id_.toString();
  if (lac_id) {
    web_socket_->sendTelemetry(telemetry_doc, nullptr, lac_id);
  } else {
    web_socket_->sendTelemetry(telemetry_doc, &getTaskID());
  }
}

bool SetValue::registered_ = TaskFactory::registerTask(type(), factory);

BaseTask* SetValue::factory(const ServiceGetters& services,
                            const JsonObjectConst& parameters,
                            Scheduler& scheduler) {
  Input input;
  populateInput(parameters, input);
  return new SetValue(scheduler, input);
}

const __FlashStringHelper* SetValue::send_data_point_key_ = FPSTR("send_dp");
const __FlashStringHelper* SetValue::send_data_point_key_error_ =
    FPSTR("Wrong property: send_dp (bool)");

}  // namespace set_value
}  // namespace tasks
}  // namespace inamata