#ifndef MINIMAL_BUILD
#include "gp8xxx.h"

#include "peripheral/peripheral_factory.h"

namespace inamata {
namespace peripheral {
namespace peripherals {
namespace gp8xxx {

GP8XXX::GP8XXX(const JsonObjectConst& parameters)
    : I2CAbstractPeripheral(parameters) {
  // If the base class constructor failed, abort the constructor
  if (!isValid()) {
    return;
  }

  data_point_type_ = getDataPointType(parameters);
  if (!data_point_type_.isValid()) {
    setInvalid(utils::ValueUnit::data_point_type_key_error);
    return;
  }

  // Choose the GP8XXX variant (GP8503, GP8211S, GP8512, GP8413, GP8302, GP8403)
  JsonVariantConst input_type = parameters[variant_key_];
  if (!input_type.is<const char*>()) {
    setInvalid(variant_key_error_);
    return;
  }

  // Get and check the resolution and I2C address depending on the variant
  uint16_t resolution;
  int i2c_address = -1;
  if (input_type == F("GP8503")) {
    variant_ = Variant::kGP8503;
    resolution = RESOLUTION_12_BIT;
    i2c_address = DFGP8XXX_I2C_DEVICEADDR;
  } else if (input_type == F("GP8211S")) {
    variant_ = Variant::kGP8211S;
    resolution = RESOLUTION_15_BIT;
    i2c_address = DFGP8XXX_I2C_DEVICEADDR;
  } else if (input_type == F("GP8512")) {
    variant_ = Variant::kGP8512;
    resolution = RESOLUTION_15_BIT;
    i2c_address = DFGP8XXX_I2C_DEVICEADDR;
  } else if (input_type == F("GP8413")) {
    variant_ = Variant::kGP8413;
    resolution = RESOLUTION_15_BIT;
    i2c_address = parseI2CAddress(parameters[i2c_address_key_]);
  } else if (input_type == F("GP8302")) {
    variant_ = Variant::kGP8302;
    resolution = RESOLUTION_12_BIT;
    i2c_address = DFGP8XXX_I2C_DEVICEADDR;
  } else if (input_type == F("GP8403")) {
    variant_ = Variant::kGP8403;
    resolution = RESOLUTION_12_BIT;
    i2c_address = parseI2CAddress(parameters[i2c_address_key_]);
  } else {
    setInvalid(variant_key_error_);
    return;
  }

  if (i2c_address < 0) {
    setInvalid(i2c_address_key_error_);
    return;
  }
  i2c_address_ = i2c_address;

  // Do a preliminary check to see if the device is connected to the bus
  if (!isDeviceConnected(i2c_address_)) {
    setInvalid(missingI2CDeviceError(i2c_address_));
    return;
  }

  // Initialize the driver with the correct Wire (I2C) interface
  driver_ = std::unique_ptr<DFRobot_GP8XXX_IIC>(
      new DFRobot_GP8XXX_IIC(resolution, i2c_address_, getWire()));
}

const String& GP8XXX::getType() const { return type(); }

const String& GP8XXX::type() {
  static const String name{"GP8XXX"};
  return name;
}

void GP8XXX::setValue(utils::ValueUnit value_unit) {
  if (value_unit.data_point_type != data_point_type_) {
    TRACELN(F("Mismatched DPT"));
    return;
  }
  value_ = value_unit.value;
  setOutput();
}

void GP8XXX::setOverride(float value) {
  override_value_ = value;
  setOutput();
}

void GP8XXX::clearOverride() {
  override_value_ = NAN;
  setOutput();
}

void GP8XXX::setOutput() {
  float value = value_;
  if (!isnan(override_value_)) {
    value = override_value_;
  }

  // For GP8302 0 to 4095 correspond to current ranges of 0-25mA
  if (variant_ == Variant::kGP8302) {
    const float max_value = 0.025;  // 0.025 A = 25mA
    const float resolution = RESOLUTION_12_BIT;
    const float clamped_value = std::fmax(0, std::fmin(value, max_value));
    const float dac_value = clamped_value * resolution / max_value;
    driver_->setDACOutVoltage(roundf(dac_value));
  }
  // TODO: logic for other variants
}

std::shared_ptr<Peripheral> GP8XXX::factory(const ServiceGetters& services,
                                            const JsonObjectConst& parameters) {
  return std::make_shared<GP8XXX>(parameters);
}

bool GP8XXX::registered_ = PeripheralFactory::registerFactory(type(), factory);

bool GP8XXX::capability_set_value_ =
    capabilities::SetValue::registerType(type());

}  // namespace gp8xxx
}  // namespace peripherals
}  // namespace peripheral
}  // namespace inamata

#endif