#include "modbus_client_input.h"

#include "peripheral/peripheral_factory.h"
#include "utils/error_store.h"

namespace inamata {
namespace peripheral {
namespace peripherals {
namespace modbus {

using namespace std::placeholders;

ModbusClientInput::ModbusClientInput(const JsonObjectConst& parameters)
    : ModbusClientAbstractPeripheral(parameters) {
  // If the base class constructor failed, abort the constructor
  if (!isValid()) {
    return;
  }

  JsonVariantConst server_id_json = parameters[server_id_key_];
  if (!server_id_json.is<float>()) {
    setInvalid(ErrorStore::genMissingProperty(server_id_key_,
                                              ErrorStore::KeyType::kFloat));
    return;
  }
  server_id_ = server_id_json.as<uint8_t>();

  JsonVariantConst read_address_json = parameters[address_key_];
  if (!read_address_json.is<float>()) {
    setInvalid(ErrorStore::genMissingProperty(address_key_,
                                              ErrorStore::KeyType::kFloat));
    return;
  }
  read_address_ = read_address_json.as<uint16_t>();

  JsonVariantConst read_size_json = parameters[read_size_key_];
  if (!read_size_json.is<float>()) {
    setInvalid(ErrorStore::genMissingProperty(read_size_key_,
                                              ErrorStore::KeyType::kFloat));
    return;
  }
  read_size_ = read_size_json.as<uint16_t>();

  JsonArrayConst inputs_json = parameters[inputs_key_];
  if (inputs_json.size() == 0) {
    setInvalid(ErrorStore::genMissingProperty(inputs_key_,
                                              ErrorStore::KeyType::kArray));
    return;
  }
  inputs_.resize(inputs_json.size());
  for (int i = 0; i < inputs_json.size(); i++) {
    Input& input = inputs_[i];
    JsonObjectConst input_json = inputs_json[i];

    input.data_point_type = getDataPointType(input_json);
    if (!input.data_point_type.isValid()) {
      setInvalid(utils::ValueUnit::data_point_type_key_error);
      return;
    }
    JsonVariantConst offset_json = input_json[offset_key_];
    if (!offset_json.is<float>()) {
      setInvalid(ErrorStore::genMissingProperty(offset_key_,
                                                ErrorStore::KeyType::kFloat));
      return;
    }
    input.offset = offset_json.as<uint16_t>();
    JsonVariantConst m_json = input_json[m_key_];
    if (m_json.is<float>()) {
      input.m = m_json.as<float>();
    }
    JsonVariantConst b_json = input_json[b_key_];
    if (b_json.is<float>()) {
      input.b = b_json.as<float>();
    }
  }
}

const String& ModbusClientInput::getType() const { return type(); }

const String& ModbusClientInput::type() {
  static const String name{"ModbusClientInput"};
  return name;
}

capabilities::StartMeasurement::Result ModbusClientInput::startMeasurement(
    const JsonVariantConst& parameters) {
  // If another task called startMeasurement don't start another request but
  // return the expected wait time. The results are cached, so will not result
  // in different getValues returns.
  if (!ready_) {
    return {.wait = measurement_wait_};
  }

  ready_ = false;
  uint32_t request_token;
  Modbus::Error error = adapter_->addReadRequest(
      server_id_, read_address_, read_size_,
      std::bind(&ModbusClientInput::handleResponse, this, _1), &request_token);

  if (error != Modbus::SUCCESS) {
    ErrorResult result(type(), (const char*)ModbusError(error));
    return {.wait = {}, .error = result};
  }
  return {.wait = measurement_wait_};
}

capabilities::StartMeasurement::Result ModbusClientInput::handleMeasurement() {
  if (ready_) {
    return {};
  }
  return {.wait = measurement_wait_};
}

capabilities::GetValues::Result ModbusClientInput::getValues() {
  if (!ready_) {
    return {.values = {}, .error = ErrorResult(type(), F("Not ready"))};
  }
  Modbus::Error error = response_.getError();
  if (error != Modbus::Error::SUCCESS) {
    return {.values = {},
            .error = ErrorResult(type(), (const char*)ModbusError(error))};
  }

  // First value is on pos 3, after server ID, function code and length byte
  const uint16_t header_offset = 3;

  // Reserve a ValueUnit for each input
  std::vector<utils::ValueUnit> values;
  values.reserve(inputs_.size());

  for (const auto& input : inputs_) {
    uint16_t raw_value;
    const uint16_t offset = input.offset * sizeof(uint16_t) + header_offset;
    if (offset + sizeof(uint16_t) > response_.size()) {
      return {.values = {}, .error = ErrorResult(type(), F("Msg OOB"))};
    }
    response_.get(offset, raw_value);
    float value = raw_value * input.m + input.b;
    values.emplace_back(value, input.data_point_type);
  }

  return {.values = values};
}

void ModbusClientInput::handleResponse(ModbusMessage& response) {
  ready_ = true;
  response_ = response;
  response_error_ = response.getError();

  if (response_error_ != Modbus::Error::SUCCESS) {
#ifdef ENABLE_TRACE
    ModbusError modbus_error(response_error_);
    TRACEF("Error: %02X - %s\n", (int)modbus_error, (const char*)modbus_error);
#endif
    // TODO: Return?
    return;
  }
}

std::shared_ptr<Peripheral> ModbusClientInput::factory(
    const ServiceGetters& services, const JsonObjectConst& parameters) {
  return std::make_shared<ModbusClientInput>(parameters);
}

bool ModbusClientInput::registered_ =
    PeripheralFactory::registerFactory(type(), factory);

bool ModbusClientInput::capability_get_values_ =
    capabilities::GetValues::registerType(type());

}  // namespace modbus
}  // namespace peripherals
}  // namespace peripheral
}  // namespace inamata