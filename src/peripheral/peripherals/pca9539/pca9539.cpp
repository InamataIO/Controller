#ifndef MINIMAL_BUILD
#include "pca9539.h"

#include "peripheral/peripheral_factory.h"
#include "utils/error_store.h"

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

  int i2c_address = parseI2CAddress(parameters[i2c_address_key_]);
  if (i2c_address < 0) {
    setInvalid(i2c_address_key_error_);
    return;
  }
  driver_ = ::PCA9539(i2c_address, getWire());

  JsonVariantConst reset_json = parameters["reset"];
  if (!reset_json.is<uint8_t>()) {
    setInvalid("Missing reset");
    return;
  }
  reset_pin_ = reset_json;
  pinMode(reset_pin_, OUTPUT);
  digitalWrite(reset_pin_, HIGH);  // Enable PCA9539
  delay(1);  // Minimum is 400 ns (reset time) + 6ns (pulse)

  // Do a preliminary check to see if the device is connected to the bus
  if (!isDeviceConnected(i2c_address)) {
    setInvalid(missingI2CDeviceError(i2c_address));
    return;
  }

  // Reset again over I2C commands
  driver_.reset();

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

const String& PCA9539::getType() const { return type(); }

const String& PCA9539::type() {
  static const String name{"PCA9539"};
  return name;
}

capabilities::GetValues::Result PCA9539::getValues() {
  capabilities::GetValues::Result result;
  result.values.reserve(inputs_.size());
  const uint16_t state = driver_.readGPIO();
  for (const IO& io : inputs_) {
    bool value = (state & (1 << io.pin)) != 0;
    if (active_low_in_) {
      value = !value;
    }
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