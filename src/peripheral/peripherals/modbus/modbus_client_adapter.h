#pragma once

#include <ArduinoJson.h>
#include <ModbusClientRTU.h>

#include <memory>

#include "managers/service_getters.h"
#include "peripheral/peripheral.h"

namespace inamata {
namespace peripheral {
namespace peripherals {
namespace modbus {

class ModbusClientAdapter : public Peripheral {
 public:
  ModbusClientAdapter(const JsonObjectConst& parameters);
  virtual ~ModbusClientAdapter() = default;

  // Type registration in the peripheral factory
  const String& getType() const final;
  static const String& type();

  Modbus::Error addReadRequest(
      const uint8_t server_id, const uint16_t read_address,
      const uint16_t read_size,
      std::function<void(ModbusMessage& response)> response_callback,
      uint32_t* const request_token);

  Modbus::Error addWriteRequest(
      const uint8_t server_id, const uint16_t write_address,
      const uint16_t value,
      std::function<void(ModbusMessage& response)> response_callback,
      uint32_t* const request_token);

 private:
  Modbus::Error preRequest();
  void postRequest(
      Modbus::Error error,
      std::function<void(ModbusMessage& response)> response_callback,
      uint32_t* const request_token);

  void modbusResponseHandler(ModbusMessage response, uint32_t token);

  static std::shared_ptr<Peripheral> factory(const ServiceGetters& services,
                                             const JsonObjectConst& parameters);
  static bool registered_;

  std::unique_ptr<ModbusClientRTU> driver_;
  std::vector<uint16_t> values_;

  uint32_t request_token_ = 0;

  std::map<uint32_t, std::function<void(ModbusMessage& response)>> callbacks_;
};

}  // namespace modbus
}  // namespace peripherals
}  // namespace peripheral
}  // namespace inamata