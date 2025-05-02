#ifndef MINIMAL_BUILD
#include "pca9539.h"

#include "peripheral/peripheral_factory.h"

namespace inamata {
namespace peripheral {
namespace peripherals {
namespace pca9539 {

PCA9539::PCA9539(const JsonObjectConst& parameters)
    : I2CAbstractPeripheral(parameters), driver_(default_i2c_address_) {
  // If the base class constructor failed, abort the constructor
  if (!isValid()) {
    return;
  }

  // Get and check the data point type
  data_point_type_ = utils::UUID(parameters[data_point_type_key_]);
  if (!data_point_type_.isValid()) {
    setInvalid(data_point_type_key_error_);
    return;
  }

  // Do a preliminary check to see if the device is connected to the bus
  if (!isDeviceConnected(default_i2c_address_)) {
    setInvalid(missingI2CDeviceError(default_i2c_address_));
    return;
  }

  driver_ = ::PCA9539(default_i2c_address_, getWire());

  JsonArrayConst inputs = parameters["inputs"].as<JsonArrayConst>();
  inputs_.reserve(inputs.size());
  JsonArrayConst outputs = parameters["outputs"].as<JsonArrayConst>();
  outputs_.reserve(outputs.size());

  for (JsonObjectConst input : inputs) {
    inputs_.emplace_back(input["pin"], utils::UUID(input["dpt"]));
  }
  for (JsonObjectConst output : outputs) {
    outputs_.emplace_back(output["pin"], utils::UUID(output["dpt"]));
  }
}

const String& PCA9539::getType() const { return type(); }

const String& PCA9539::type() {
  static const String name{"PCA9539"};
  return name;
}

capabilities::GetValues::Result PCA9539::getValues() {
  capabilities::GetValues::Result result;
  const uint16_t state = driver_.readGPIO();
  for (const IO& io : inputs_) {
    const bool value = (state & (1 << io.pin)) != 0;
    result.values.push_back(utils::ValueUnit(value, io.dpt));
  }

  return result;
}

void PCA9539::setValue(utils::ValueUnit value_unit) {
  // Search for the IO entry with the matching DPT
  auto it = std::find_if(outputs_.begin(), outputs_.end(),
                         [value_unit](const IO& io) {
                           return io.dpt == value_unit.data_point_type;
                         });
  if (it == outputs_.end()) {
    return;
  }
  // Use the IO entry's pin and set the value (on/off)
  driver_.digitalWrite(it->pin, bool(value_unit.value));
}

std::shared_ptr<Peripheral> PCA9539::factory(
    const ServiceGetters& services, const JsonObjectConst& parameters) {
  return std::make_shared<PCA9539>(parameters);
}

bool PCA9539::registered_ = PeripheralFactory::registerFactory(type(), factory);

bool PCA9539::capability_get_values_ =
    capabilities::GetValues::registerType(type());
bool PCA9539::capability_set_value_ =
    capabilities::SetValue::registerType(type());

}  // namespace pca9539
}  // namespace peripherals
}  // namespace peripheral
}  // namespace inamata
#endif