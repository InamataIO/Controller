#include "get_values_task.h"

#include "managers/services.h"
#include "peripheral/peripheral.h"

namespace inamata {
namespace tasks {
namespace get_values_task {

GetValuesTask::GetValuesTask(const ServiceGetters& services,
                             Scheduler& scheduler, const Input& input)
    : BaseTask(scheduler, input),
      peripheral_id_(input.peripheral_id),
      web_socket_(services.getWebSocket()) {
  // Check if the init from the JSON doc was successful
  if (!isValid()) {
    return;
  }

  if (web_socket_ == nullptr) {
    setInvalid(services.web_socket_nullptr_error_);
    return;
  }

  // Get the UUID to later find the pointer to the peripheral object
  if (!peripheral_id_.isValid()) {
    setInvalid(peripheral_key_error_);
    return;
  }

  // Search for the peripheral for the given name
  auto peripheral =
      Services::getPeripheralController().getPeripheral(input.peripheral_id);
  if (!peripheral) {
    setInvalid(peripheral::Peripheral::peripheralNotFoundError(peripheral_id_));
    return;
  }

  // Check that the peripheral supports the GetValues interface capability
  peripheral_ = std::dynamic_pointer_cast<peripheral::capabilities::GetValues>(
      peripheral);
  if (!peripheral_) {
    setInvalid(peripheral::capabilities::GetValues::invalidTypeError(
        peripheral_id_, peripheral));
    return;
  }
}

std::shared_ptr<peripheral::capabilities::GetValues>
GetValuesTask::getPeripheral() {
  return peripheral_;
}

const utils::UUID& GetValuesTask::getPeripheralUUID() const {
  return peripheral_id_;
}

void GetValuesTask::populateInput(const JsonObjectConst& parameters,
                                  Input& input) {
  BaseTask::populateInput(parameters, input);

  // Get the UUID to later find the pointer to the peripheral object
  JsonVariantConst peripheral_id = parameters[peripheral_key_];
  if (!peripheral_id.isNull()) {
    input.peripheral_id = peripheral_id;
  }
}

void GetValuesTask::sendTelemetry(
    peripheral::capabilities::GetValues::Result& result) {
  // Clear the JSON doc, then insert value units and peripheral UUID
  JsonDocument doc_out;
  JsonObject result_object = doc_out.to<JsonObject>();
  packageValues(result, result_object);

  // Send the result to the server
  web_socket_->sendTelemetry(result_object, &getTaskID());
}

void GetValuesTask::packageValues(
    peripheral::capabilities::GetValues::Result& result,
    JsonObject& telemetry) {
  // Create an array for the value units and get them from the peripheral
  JsonArray value_units_doc =
      telemetry[utils::ValueUnit::data_points_key].to<JsonArray>();

  // Create a JSON object representation for each value unit in the array
  for (const auto& value_unit : result.values) {
    JsonObject value_unit_object = value_units_doc.add<JsonObject>();
    value_unit_object[utils::ValueUnit::value_key] = value_unit.value;
    value_unit_object[utils::ValueUnit::data_point_type_key] =
        value_unit.data_point_type.toString();
  }

  // Add the peripheral UUID to the result
  telemetry[peripheral_key_] = peripheral_id_.toString();
}

const __FlashStringHelper* GetValuesTask::threshold_key_ = FPSTR("threshold");
const __FlashStringHelper* GetValuesTask::threshold_key_error_ =
    FPSTR("Missing property: threshold (float)");
const __FlashStringHelper* GetValuesTask::trigger_type_key_ =
    FPSTR("trigger_type");
const __FlashStringHelper* GetValuesTask::trigger_type_key_error_ =
    FPSTR("Missing property: trigger_type (string)");
const __FlashStringHelper* GetValuesTask::interval_ms_key_ =
    FPSTR("interval_ms");
const __FlashStringHelper* GetValuesTask::interval_ms_key_error_ =
    FPSTR("Missing property: interval_ms (unsigned int)");
const __FlashStringHelper* GetValuesTask::duration_ms_key_ =
    FPSTR("duration_ms");

}  // namespace get_values_task
}  // namespace tasks
}  // namespace inamata
