#include "set_value.h"

#include "managers/services.h"
#include "tasks/task_factory.h"

namespace inamata {
namespace tasks {
namespace set_value {

SetValue::SetValue(const ServiceGetters& services,
                   const JsonObjectConst& parameters, Scheduler& scheduler)
    : BaseTask(scheduler, parameters) {
  // Abort if the base class failed initialization
  if (!isValid()) {
    return;
  }

  web_socket_ = services.getWebSocket();
  if (web_socket_ == nullptr) {
    setInvalid(services.web_socket_nullptr_error_);
    return;
  }

  // Get the UUID to later find the pointer to the peripheral object
  peripheral_id_ = parameters[peripheral_key_];
  if (!peripheral_id_.isValid()) {
    setInvalid(peripheral_key_error_);
    return;
  }

  // Search for the peripheral for the given name
  auto peripheral =
      Services::getPeripheralController().getPeripheral(peripheral_id_);
  if (!peripheral) {
    setInvalid(peripheral_not_found_error_);
    return;
  }

  // Check that the peripheral supports the SetValue interface capability
  peripheral_ =
      std::dynamic_pointer_cast<peripheral::capabilities::SetValue>(peripheral);
  if (!peripheral_) {
    setInvalid(peripheral::capabilities::SetValue::invalidTypeError(
        peripheral_id_, peripheral));
    return;
  }

  // Get the value
  JsonVariantConst value = parameters[utils::ValueUnit::value_key];
  if (!value.is<float>()) {
    setInvalid(value_unit_.value_key_error);
    return;
  }

  // Get the unit of the value
  utils::UUID data_point_type(
      parameters[utils::ValueUnit::data_point_type_key]);
  if (!data_point_type.isValid()) {
    setInvalid(value_unit_.data_point_type_key_error);
    return;
  }

  // Save the ValueUnit
  value_unit_ =
      utils::ValueUnit{.value = value, .data_point_type = data_point_type};

  // Send the set DPT as a telemetry message
  JsonVariantConst send_data_point = parameters[send_data_point_key_];
  if (send_data_point.is<bool>()) {
    send_data_point_ = send_data_point;
  } else {
    send_data_point_ = true;
  }

  enable();
}

const String& SetValue::getType() const { return type(); }

const String& SetValue::type() {
  static const String name{"SetValue"};
  return name;
}

bool SetValue::TaskCallback() {
  peripheral_->setValue(value_unit_);
  if (send_data_point_) {
    doc_out.clear();
    JsonObject telemetry_doc = doc_out.to<JsonObject>();
    JsonArray value_units_doc =
        telemetry_doc.createNestedArray(utils::ValueUnit::data_points_key);
    JsonObject value_unit_object = value_units_doc.createNestedObject();
    value_unit_object[utils::ValueUnit::value_key] = value_unit_.value;
    value_unit_object[utils::ValueUnit::data_point_type_key] =
        value_unit_.data_point_type.toString();
    telemetry_doc[peripheral_key_] = peripheral_id_.toString();
    web_socket_->sendTelemetry(getTaskID(), telemetry_doc);
  }
  return false;
}

bool SetValue::registered_ = TaskFactory::registerTask(type(), factory);

BaseTask* SetValue::factory(const ServiceGetters& services,
                            const JsonObjectConst& parameters,
                            Scheduler& scheduler) {
  return new SetValue(services, parameters, scheduler);
}

const __FlashStringHelper* SetValue::send_data_point_key_ = FPSTR("send_dp");
const __FlashStringHelper* SetValue::send_data_point_key_error_ =
    FPSTR("Wrong property: send_dp (bool)");

}  // namespace set_value
}  // namespace tasks
}  // namespace inamata