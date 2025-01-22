#include "modbus_client_abstract_peripheral.h"

#include "managers/services.h"

namespace inamata {
namespace peripheral {
namespace peripherals {
namespace modbus {

ModbusClientAbstractPeripheral::ModbusClientAbstractPeripheral(
    const JsonObjectConst& parameters) {
  utils::UUID adapter_uuid(parameters[modbus_client_adapter_key_]);
  if (!adapter_uuid.isValid()) {
    setInvalid(ErrorStore::genMissingProperty(modbus_client_adapter_key_,
                                              ErrorStore::KeyType::kUUID));
    return;
  }

  std::shared_ptr<Peripheral> peripheral =
      Services::getPeripheralController().getPeripheral(adapter_uuid);
  if (!peripheral) {
    setInvalid(peripheralNotFoundError(adapter_uuid));
    return;
  }

  // Since the UUID is specified externally, check the type
  const String& type = ModbusClientAdapter::type();
  if (peripheral->getType() == type && peripheral->isValid()) {
    adapter_ = std::static_pointer_cast<ModbusClientAdapter>(peripheral);
  } else {
    setInvalid(ErrorStore::genNotAValid(adapter_uuid, type));
    return;
  }
}

const __FlashStringHelper* ModbusClientAbstractPeripheral::server_id_key_ =
    FPSTR("server");
const __FlashStringHelper* ModbusClientAbstractPeripheral::address_key_ =
    FPSTR("address");
const __FlashStringHelper* ModbusClientAbstractPeripheral::read_size_key_ =
    FPSTR("size");
const __FlashStringHelper* ModbusClientAbstractPeripheral::inputs_key_ =
    FPSTR("inputs");
const __FlashStringHelper* ModbusClientAbstractPeripheral::outputs_key_ =
    FPSTR("outputs");
const __FlashStringHelper* ModbusClientAbstractPeripheral::offset_key_ =
    FPSTR("offset");
const __FlashStringHelper* ModbusClientAbstractPeripheral::m_key_ = FPSTR("m");
const __FlashStringHelper* ModbusClientAbstractPeripheral::b_key_ = FPSTR("b");

}  // namespace modbus
}  // namespace peripherals
}  // namespace peripheral
}  // namespace inamata