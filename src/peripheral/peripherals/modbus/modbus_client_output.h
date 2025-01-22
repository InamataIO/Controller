#pragma once

#include <ArduinoJson.h>

#include "managers/service_getters.h"
#include "peripheral/capabilities/set_value.h"
#include "peripheral/peripheral.h"
#include "peripheral/peripherals/modbus/modbus_client_abstract_peripheral.h"

namespace inamata {
namespace peripheral {
namespace peripherals {
namespace modbus {

class ModbusClientOutput : public ModbusClientAbstractPeripheral,
                           public capabilities::SetValue {
 public:
  ModbusClientOutput(const JsonObjectConst& parameters);
  virtual ~ModbusClientOutput() = default;

  // Type registration in the peripheral factory
  const String& getType() const final;
  static const String& type();

  void setValue(utils::ValueUnit value_unit) final;

 private:
  /**
   * data_point_type: UUID of the data point type
   * address:
   * m: slope/gradient in the slope-intercept equation
   * b: intercept/y-axis offset in the slope-intercept equation
   *
   * y = mx + b is applied to the extracted value
   */
  struct Output {
    utils::UUID data_point_type{nullptr};
    uint16_t address = 0;
    float m = 1;
    float b = 0;
  };

  void handleResponse(ModbusMessage& response);

  static std::shared_ptr<Peripheral> factory(const ServiceGetters& services,
                                             const JsonObjectConst& parameters);
  static bool registered_;
  static bool capability_set_value_;

  std::vector<Output> outputs_;

  bool ready_;
  uint8_t server_id_;

  Modbus::Error request_error_;
};

}  // namespace modbus
}  // namespace peripherals
}  // namespace peripheral
}  // namespace inamata