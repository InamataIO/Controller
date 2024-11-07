#ifndef MINIMAL_BUILD
#include "sgp40.h"

#include "peripheral/peripheral_factory.h"

namespace inamata {
namespace peripheral {
namespace peripherals {
namespace sgp40 {

SGP40::SGP40(const JsonObjectConst& parameters)
    : I2CAbstractPeripheral(parameters) {
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
  if (!isDeviceConnected(DFRobot_SGP40_ICC_ADDR)) {
    setInvalid(missingI2CDeviceError(DFRobot_SGP40_ICC_ADDR));
    return;
  }

  driver_ = DFRobot_SGP40(getWire());
  bool setup_success = driver_.begin();
  if (!setup_success) {
    setInvalid(chip_init_error);
    return;
  }
}

const String& SGP40::getType() const { return type(); }

const String& SGP40::type() {
  static const String name{"SGP40"};
  return name;
}

capabilities::Calibrate::Result SGP40::startCalibration(
    const JsonObjectConst& parameters) {
  // Get the calibration temperature and humidity
  JsonVariantConst temperature = parameters[temperature_c_key_];
  if (!temperature.is<float>()) {
    return {.wait = {}, .error = ErrorResult(type(), temperature_c_key_error_)};
  }
  JsonVariantConst humidity = parameters[humidity_rh_key_];
  if (!humidity.is<float>()) {
    return {.wait = {}, .error = ErrorResult(type(), humidity_rh_key_error_)};
  }

  driver_.setRhT(humidity, temperature);
  return capabilities::Calibrate::Result();
}

capabilities::Calibrate::Result SGP40::startCalibration(
    const float humidity, const float temperature_c) {
  driver_.setRhT(humidity, temperature_c);
  return capabilities::Calibrate::Result();
}

capabilities::Calibrate::Result SGP40::handleCalibration() {
  return capabilities::Calibrate::Result();
}

capabilities::GetValues::Result SGP40::getValues() {
  capabilities::GetValues::Result result;
  result.values.push_back(
      utils::ValueUnit{.value = float(driver_.getVoclndex()),
                       .data_point_type = data_point_type_});
  return result;
}

std::shared_ptr<Peripheral> SGP40::factory(const ServiceGetters& services,
                                           const JsonObjectConst& parameters) {
  return std::make_shared<SGP40>(parameters);
}

bool SGP40::registered_ = PeripheralFactory::registerFactory(type(), factory);

bool SGP40::capability_get_values_ =
    capabilities::GetValues::registerType(type());

const __FlashStringHelper* SGP40::chip_init_error =
    FPSTR("SGP40 init test failed");

}  // namespace sgp40
}  // namespace peripherals
}  // namespace peripheral
}  // namespace inamata
#endif