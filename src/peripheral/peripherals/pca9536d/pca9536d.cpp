#ifndef MINIMAL_BUILD
#include "pca9536d.h"

#include "peripheral/peripheral_factory.h"
#include "utils/error_store.h"

namespace inamata {
namespace peripheral {
namespace peripherals {
namespace pca9536d {

PCA9536D::PCA9536D(const JsonObjectConst& parameters)
    : I2CAbstractPeripheral(parameters) {
  // If the base class constructor failed, abort the constructor
  if (!isValid()) {
    return;
  }

  // Do a preliminary check to see if the device is connected to the bus
  const bool connected = driver_.begin(*getWire());
  if (!connected) {
    setInvalid(missingI2CDeviceError(PCA9536_ADDRESS));
    return;
  }

  JsonArrayConst inputs = parameters["inputs"].as<JsonArrayConst>();
  inputs_.reserve(inputs.size());
  JsonArrayConst outputs = parameters["outputs"].as<JsonArrayConst>();
  outputs_.reserve(outputs.size());

  for (JsonObjectConst input : inputs) {
    inputs_.emplace_back(input["pin"], utils::UUID(input["dpt"]));
    driver_.pinMode(input["pin"], INPUT);
  }
  for (JsonObjectConst output : outputs) {
    outputs_.emplace_back(output["pin"], utils::UUID(output["dpt"]));
    driver_.pinMode(output["pin"], OUTPUT);
  }

  // Set if it is an active low peripheral. Default is active high
  JsonVariantConst active_low_in = parameters[active_low_in_key_];
  if (!active_low_in.isNull() && !active_low_in.is<bool>()) {
    setInvalid(ErrorStore::genMissingProperty(
        active_low_in_key_, ErrorStore::KeyType::kBool, true));
    return;
  }
  // If null, returns false. If bool, returns true or false
  if (active_low_in.as<bool>()) {
    active_low_in_ = true;
  }
}

const String& PCA9536D::getType() const { return type(); }

const String& PCA9536D::type() {
  static const String name{"PCA9536D"};
  return name;
}

capabilities::GetValues::Result PCA9536D::getValues() {
  capabilities::GetValues::Result result;
  result.values.reserve(inputs_.size());
  const uint8_t state = driver_.readReg();
  for (const IO& io : inputs_) {
    bool value = (state & (1 << io.pin)) != 0;
    if (active_low_in_) {
      value = !value;
    }
    result.values.push_back(utils::ValueUnit(value, io.dpt));
  }

  return result;
}

void PCA9536D::setValue(utils::ValueUnit value_unit) {
  // Search for the IO entry with the matching DPT
  auto it = std::find_if(outputs_.begin(), outputs_.end(),
                         [value_unit](const IO& io) {
                           return io.dpt == value_unit.data_point_type;
                         });
  if (it == outputs_.end()) {
    return;
  }
  // Use the IO entry's pin and set the value (on/off)
  driver_.digitalWrite(it->pin, value_unit.value > 0.5);
}

std::shared_ptr<Peripheral> PCA9536D::factory(
    const ServiceGetters& services, const JsonObjectConst& parameters) {
  return std::make_shared<PCA9536D>(parameters);
}

bool PCA9536D::registered_ =
    PeripheralFactory::registerFactory(type(), factory);

bool PCA9536D::capability_get_values_ =
    capabilities::GetValues::registerType(type());
bool PCA9536D::capability_set_value_ =
    capabilities::SetValue::registerType(type());

}  // namespace pca9536d
}  // namespace peripherals
}  // namespace peripheral
}  // namespace inamata
#endif