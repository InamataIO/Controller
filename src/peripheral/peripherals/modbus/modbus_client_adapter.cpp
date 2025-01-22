#include "modbus_client_adapter.h"

#include "peripheral/peripheral_factory.h"
#include "utils/error_store.h"

namespace inamata {
namespace peripheral {
namespace peripherals {
namespace modbus {

ModbusClientAdapter::ModbusClientAdapter(const JsonObjectConst& parameters) {
  int rx_pin = -1;    // Receive pin
  int tx_pin = -1;    // Transmit pin
  int dere_pin = -1;  // Flow control (drive enable/receive enable) pin aka RTS

  // Only assign value if it exists and is a Number (else 0 is returned)
  JsonVariantConst rx_pin_json = parameters[rx_key_];
  if (!rx_pin_json.isNull() && rx_pin_json.is<float>()) {
    rx_pin = rx_pin_json.as<int>();
  }
  JsonVariantConst tx_pin_json = parameters[tx_key_];
  if (!tx_pin_json.isNull() && tx_pin_json.is<float>()) {
    tx_pin = tx_pin_json.as<int>();
  }
  JsonVariantConst dere_pin_json = parameters[dere_key_];
  if (!dere_pin_json.isNull() && dere_pin_json.is<float>()) {
    dere_pin = dere_pin_json.as<int>();
  }

  // baud rate (4800, 9600, 115200, ...)
  JsonVariantConst baud_rate_json = parameters[baud_rate_key_];
  if (!baud_rate_json.is<float>()) {
    setInvalid(ErrorStore::genMissingProperty(baud_rate_key_,
                                              ErrorStore::KeyType::kFloat));
    return;
  }

  // Initialize serial and Modbus interface
  RTUutils::prepareHardwareSerial(Serial1);
  Serial1.begin(baud_rate_json.as<unsigned long>(), SERIAL_8N1, rx_pin, tx_pin);
  driver_ = std::unique_ptr<ModbusClientRTU>(new ModbusClientRTU(dere_pin));
  driver_->onResponseHandler(
      std::bind(&ModbusClientAdapter::modbusResponseHandler, this, _1, _2));
  driver_->begin(Serial1);
}

const String& ModbusClientAdapter::getType() const { return type(); }

const String& ModbusClientAdapter::type() {
  static const String name{"ModbusClientAdapter"};
  return name;
}

Modbus::Error ModbusClientAdapter::addReadRequest(
    const uint8_t server_id, const uint16_t read_address,
    const uint16_t read_size,
    std::function<void(ModbusMessage& response)> response_callback,
    uint32_t* const request_token) {
  Modbus::Error error = preRequest();
  if (error != Modbus::Error::SUCCESS) {
    return error;
  }

  error = driver_->addRequest(++request_token_, server_id, READ_HOLD_REGISTER,
                              read_address, read_size);

  postRequest(error, response_callback, request_token);
  return error;
}

Modbus::Error ModbusClientAdapter::addWriteRequest(
    const uint8_t server_id, const uint16_t write_address, const uint16_t value,
    std::function<void(ModbusMessage& response)> response_callback,
    uint32_t* const request_token) {
  Modbus::Error error = preRequest();
  if (error != Modbus::Error::SUCCESS) {
    return error;
  }

  Modbus::Error err = driver_->addRequest(++request_token_, server_id,
                                          WRITE_COIL, write_address, value);

  postRequest(error, response_callback, request_token);
  return err;
}

Modbus::Error ModbusClientAdapter::preRequest() {
  // If callbacks aren't removed, remove oldest
  if (callbacks_.size() > 10) {
    const auto& callback = callbacks_.begin();
    TRACEF("Callbacks overflow: %u\n", callback->first);
    callbacks_.erase(callback);
  }
  return Modbus::Error::SUCCESS;
}

void ModbusClientAdapter::postRequest(
    Modbus::Error error,
    std::function<void(ModbusMessage& response)> response_callback,
    uint32_t* const request_token) {
  // Save the current request token value to the output variable
  if (request_token) {
    *request_token = request_token_;
  }
  // On success register the callback, else print the error
  if (error == Modbus::SUCCESS && response_callback) {
    callbacks_[request_token_] = response_callback;
  } else {
    ModbusError e(error);
    TRACEF("Error creating request: %02X - %s\n", (int)e, (const char*)e);
  }
}

void ModbusClientAdapter::modbusResponseHandler(ModbusMessage response,
                                                uint32_t token) {
  auto callback = callbacks_.find(token);
  if (callback == callbacks_.end()) {
    TRACEF("Callback not found: %d\n", token);
    return;
  }
  if (callback->second) {
    callback->second(response);
  } else {
    TRACEF("Null callback: %d\n", token);
  }
  callbacks_.erase(callback);
}

std::shared_ptr<Peripheral> ModbusClientAdapter::factory(
    const ServiceGetters& services, const JsonObjectConst& parameters) {
  return std::make_shared<ModbusClientAdapter>(parameters);
}

bool ModbusClientAdapter::registered_ =
    PeripheralFactory::registerFactory(type(), factory);

}  // namespace modbus
}  // namespace peripherals
}  // namespace peripheral
}  // namespace inamata