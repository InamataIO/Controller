#pragma once

#include <ArduinoJson.h>

#include "peripheral/peripheral.h"
#include "peripheral/peripherals/modbus/modbus_client_adapter.h"

namespace inamata {
namespace peripheral {
namespace peripherals {
namespace modbus {

class ModbusClientAbstractPeripheral : public Peripheral {
 public:
  ModbusClientAbstractPeripheral(const JsonObjectConst& parameters);
  virtual ~ModbusClientAbstractPeripheral() = default;

 protected:
  std::shared_ptr<ModbusClientAdapter> adapter_;

  static const __FlashStringHelper* server_id_key_;
  static const __FlashStringHelper* address_key_;
  static const __FlashStringHelper* read_size_key_;
  static const __FlashStringHelper* inputs_key_;
  static const __FlashStringHelper* outputs_key_;
  static const __FlashStringHelper* offset_key_;
  static const __FlashStringHelper* m_key_;
  static const __FlashStringHelper* b_key_;
};

}  // namespace modbus
}  // namespace peripherals
}  // namespace peripheral
}  // namespace inamata