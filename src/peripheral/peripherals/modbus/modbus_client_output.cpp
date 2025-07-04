#include "modbus_client_output.h"

#include "peripheral/peripheral_factory.h"
#include "utils/error_store.h"

namespace inamata {
namespace peripheral {
namespace peripherals {
namespace modbus {

using namespace std::placeholders;

ModbusClientOutput::ModbusClientOutput(const JsonObjectConst& parameters)
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

  JsonArrayConst outputs_json = parameters[outputs_key_];
  if (outputs_json.size() == 0) {
    setInvalid(ErrorStore::genMissingProperty(outputs_key_,
                                              ErrorStore::KeyType::kArray));
    return;
  }
  outputs_.resize(outputs_json.size());
  for (int i = 0; i < outputs_json.size(); i++) {
    Output& output = outputs_[i];
    JsonObjectConst input_json = outputs_json[i];

    output.data_point_type = getDataPointType(input_json);
    if (!output.data_point_type.isValid()) {
      setInvalid(utils::ValueUnit::data_point_type_key_error);
      return;
    }
    JsonVariantConst address_json = input_json[address_key_];
    if (!address_json.is<float>()) {
      setInvalid(ErrorStore::genMissingProperty(address_key_,
                                                ErrorStore::KeyType::kFloat));
      return;
    }
    output.address = address_json.as<uint16_t>();
    JsonVariantConst m_json = input_json[m_key_];
    if (m_json.is<float>()) {
      output.m = m_json.as<float>();
    }
    JsonVariantConst b_json = input_json[b_key_];
    if (b_json.is<float>()) {
      output.b = b_json.as<float>();
    }
  }
}

const String& ModbusClientOutput::getType() const { return type(); }

const String& ModbusClientOutput::type() {
  static const String name{"ModbusClientOutput"};
  return name;
}

void ModbusClientOutput::setValue(utils::ValueUnit value_unit) {
  uint32_t request_token;
  bool added_request = false;
  for (const auto& output : outputs_) {
    if (output.data_point_type == value_unit.data_point_type) {
      added_request = true;
      adapter_->addWriteRequest(
          server_id_, output.address, value_unit.value,
          std::bind(&ModbusClientOutput::handleResponse, this, _1),
          &request_token);
    }
  }
  if (!added_request) {
    TRACEF("No DPT match: %s\r\n",
           value_unit.data_point_type.toString().c_str());
  }
}

void ModbusClientOutput::handleResponse(ModbusMessage& response) {
  ready_ = true;
  if (response.getError() != Modbus::Error::SUCCESS) {
    request_error_ = response.getError();

#ifdef ENABLE_TRACE
    ModbusError modbus_error(request_error_);
    TRACEF("Error: %02X - %s\r\n", (int)modbus_error,
           (const char*)modbus_error);
#endif
    // TODO: Return?
    return;
  }

  request_error_ = Modbus::Error::SUCCESS;
}

std::shared_ptr<Peripheral> ModbusClientOutput::factory(
    const ServiceGetters& services, const JsonObjectConst& parameters) {
  return std::make_shared<ModbusClientOutput>(parameters);
}

bool ModbusClientOutput::registered_ =
    PeripheralFactory::registerFactory(type(), factory);

bool ModbusClientOutput::capability_set_value_ =
    capabilities::SetValue::registerType(type());

}  // namespace modbus
}  // namespace peripherals
}  // namespace peripheral
}  // namespace inamata