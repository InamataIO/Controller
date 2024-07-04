#ifndef MINIMAL_BUILD
#include "hdc2080.h"

#include "peripheral/peripheral_factory.h"

namespace inamata {
namespace peripheral {
namespace peripherals {
namespace hdc2080 {

HDC2080::HDC2080(const JsonObjectConst& parameters)
    : I2CAbstractPeripheral(parameters), driver_(0) {
  // If the base class constructor failed, abort the constructor
  if (!isValid()) {
    return;
  }

  // Get and check the data point type for temperature readings
  temperature_data_point_type_ =
      utils::UUID(parameters[temperature_data_point_type_key_]);
  if (!temperature_data_point_type_.isValid()) {
    setInvalid(temperature_data_point_type_key_error_);
    return;
  }

  // Get and check the data point type for humidity readings
  humidity_data_point_type_ =
      utils::UUID(parameters[humidity_data_point_type_key_]);
  if (!humidity_data_point_type_.isValid()) {
    setInvalid(humidity_data_point_type_key_error_);
    return;
  }
  // Get and check the I2C address of the BME/P280 chip
  int i2c_address = parseI2CAddress(parameters[i2c_address_key_]);
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
  driver_ = ::HDC2080(i2c_address_);

  driver_.reset();
  driver_.setMeasurementMode(TEMP_AND_HUMID);
  driver_.setRate(ONE_HZ);
  driver_.setTempRes(FOURTEEN_BIT);
  driver_.setHumidRes(FOURTEEN_BIT);

  driver_.triggerMeasurement();
}

const String& HDC2080::getType() const { return type(); }

const String& HDC2080::type() {
  static const String name{"HDC2080"};
  return name;
}

capabilities::GetValues::Result HDC2080::getValues() {
  if (!isDeviceConnected(i2c_address_)) {
    return {.values = {},
            .error = ErrorResult(type(), missingI2CDeviceError(i2c_address_))};
  }

  capabilities::GetValues::Result result;

  result.values.push_back(
      utils::ValueUnit{.value = driver_.readTemp(),
                       .data_point_type = temperature_data_point_type_});
  result.values.push_back(
      utils::ValueUnit{.value = driver_.readHumidity(),
                       .data_point_type = humidity_data_point_type_});
  return result;
}

std::shared_ptr<Peripheral> HDC2080::factory(
    const ServiceGetters& services, const JsonObjectConst& parameters) {
  return std::make_shared<HDC2080>(parameters);
}

bool HDC2080::registered_ = PeripheralFactory::registerFactory(type(), factory);

bool HDC2080::capability_get_values_ =
    capabilities::GetValues::registerType(type());

}  // namespace hdc2080
}  // namespace peripherals
}  // namespace peripheral
}  // namespace inamata
#endif