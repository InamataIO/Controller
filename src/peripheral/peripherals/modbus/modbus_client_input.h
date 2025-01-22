#pragma once

#include <ArduinoJson.h>

#include "managers/service_getters.h"
#include "peripheral/capabilities/get_values.h"
#include "peripheral/capabilities/start_measurement.h"
#include "peripheral/peripheral.h"
#include "peripheral/peripherals/modbus/modbus_client_abstract_peripheral.h"

namespace inamata {
namespace peripheral {
namespace peripherals {
namespace modbus {

class ModbusClientInput : public ModbusClientAbstractPeripheral,
                          public capabilities::GetValues,
                          public capabilities::StartMeasurement {
 public:
  ModbusClientInput(const JsonObjectConst& parameters);
  virtual ~ModbusClientInput() = default;

  // Type registration in the peripheral factory
  const String& getType() const final;
  static const String& type();

  capabilities::StartMeasurement::Result startMeasurement(
      const JsonVariantConst& parameters) final;

  capabilities::StartMeasurement::Result handleMeasurement() final;

  /**
   * Get the configured input value
   *
   * \return The read value
   */
  capabilities::GetValues::Result getValues() final;

 private:
  /**
   * data_point_type: UUID of the data point type
   * offset: offset in bytes of the message payload (excl. 3 header bytes)
   * m: slope/gradient in the slope-intercept equation
   * b: intercept/y-axis offset in the slope-intercept equation
   *
   * y = mx + b is applied to the extracted value
   *
   * Currently only uint16_t values are extracted from the Modbus response
   */
  struct Input {
    utils::UUID data_point_type{nullptr};
    uint16_t offset = 0;
    float m = 1;
    float b = 0;
  };

  void handleResponse(ModbusMessage& response);

  static std::shared_ptr<Peripheral> factory(const ServiceGetters& services,
                                             const JsonObjectConst& parameters);
  static bool registered_;
  static bool capability_get_values_;

  std::vector<Input> inputs_;

  bool ready_ = true;
  uint8_t server_id_;
  uint16_t read_address_;
  uint16_t read_size_;

  ModbusMessage response_;
  Modbus::Error response_error_;

  std::chrono::milliseconds measurement_wait_{50};
};

}  // namespace modbus
}  // namespace peripherals
}  // namespace peripheral
}  // namespace inamata