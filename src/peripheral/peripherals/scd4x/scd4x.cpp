#ifndef MINIMAL_BUILD
#include "scd4x.h"

#include "peripheral/peripheral_factory.h"
#include "utils/chrono.h"
#include "utils/error_store.h"

namespace inamata {
namespace peripheral {
namespace peripherals {
namespace scd4x {

SCD4X::SCD4X(const JsonObjectConst& parameters)
    : I2CAbstractPeripheral(parameters) {
  // If the base class constructor failed, abort the constructor
  if (!isValid()) {
    return;
  }

  co2_data_point_type_ = utils::UUID(parameters[co2_data_point_type_key_]);
  if (!co2_data_point_type_.isValid()) {
    setInvalid(ErrorStore::genMissingProperty(co2_data_point_type_key_,
                                              ErrorStore::KeyType::kUUID));
    return;
  }

  // Temperature and humidity are optional if dedicated DPTs are configured.
  temperature_data_point_type_ =
      utils::UUID(parameters[temperature_data_point_type_key_]);
  humidity_data_point_type_ =
      utils::UUID(parameters[humidity_data_point_type_key_]);

  JsonVariantConst i2c_address_parameter = parameters[i2c_address_key_];
  if (!i2c_address_parameter.isNull()) {
    int i2c_address = parseI2CAddress(i2c_address_parameter);
    if (i2c_address < 0) {
      setInvalid(i2c_address_key_error_);
      return;
    }
    i2c_address_ = static_cast<uint8_t>(i2c_address);
  }

  // Do a preliminary check to see if the device is connected to the bus
  if (!isDeviceConnected(i2c_address_)) {
    setInvalid(missingI2CDeviceError(i2c_address_));
    return;
  }

  driver_.begin(*getWire(), i2c_address_);
  delay(30);
  int16_t error = driver_.wakeUp();
  char errorMessage[64];
  if (error) {
    Serial.print("Error trying to execute wakeUp(): ");
    errorToString(error, errorMessage, sizeof errorMessage);
    Serial.println(errorMessage);
  }
  error = driver_.stopPeriodicMeasurement();
  if (error) {
    Serial.print("Error trying to execute stopPeriodicMeasurement(): ");
    errorToString(error, errorMessage, sizeof errorMessage);
    Serial.println(errorMessage);
  }
  error = driver_.reinit();
  if (error) {
    Serial.print("Error trying to execute reinit(): ");
    errorToString(error, errorMessage, sizeof errorMessage);
    Serial.println(errorMessage);
  }
  // Read out information about the sensor
  uint64_t serialNumber = 0;
  error = driver_.getSerialNumber(serialNumber);
  if (error) {
    Serial.print("Error trying to execute getSerialNumber(): ");
    errorToString(error, errorMessage, sizeof errorMessage);
    Serial.println(errorMessage);
    return;
  }
  Serial.print("serial number: ");
  Serial.print("0x");
  Serial.print((uint32_t)(serialNumber >> 32), HEX);
  Serial.print((uint32_t)(serialNumber & 0xFFFFFFFF), HEX);
  Serial.println();
}

const String& SCD4X::getType() const { return type(); }

const String& SCD4X::type() {
  static const String name{"SCD4X"};
  return name;
}

bool SCD4X::hasCachedMeasurement() const {
  return measurement_ready_ &&
         !utils::hasTimedOut(measurement_ready_time_, kCacheDuration);
}

std::chrono::nanoseconds SCD4X::waitUntilNextMeasurementCheck() const {
  const auto now = std::chrono::steady_clock::now();
  if (now >= next_measurement_check_time_) {
    return {};
  }
  return std::chrono::duration_cast<std::chrono::nanoseconds>(
      next_measurement_check_time_ - now);
}

capabilities::StartMeasurement::Result SCD4X::startMeasurement(
    const JsonVariantConst& parameters) {
  if (!isDeviceConnected(i2c_address_)) {
    return {.wait = {},
            .error = ErrorResult(type(), missingI2CDeviceError(i2c_address_))};
  }

  if (hasCachedMeasurement()) {
    return {.wait = {}};
  }

  if (!periodic_started_) {
    int16_t error = driver_.startPeriodicMeasurement();
    if (error != 0) {
      return {.wait = {},
              .error = ErrorResult(
                  type(), String("startPeriodicMeasurement failed: ") + error)};
    }
    periodic_started_ = true;
    measurement_ready_ = false;
    last_temperature_c_ = NAN;
    last_humidity_rh_ = NAN;
    next_measurement_check_time_ =
        std::chrono::steady_clock::now() + kInitialMeasurementWait;
    return {.wait = kInitialMeasurementWait};
  }

  measurement_ready_ = false;
  return {.wait = waitUntilNextMeasurementCheck()};
}

capabilities::StartMeasurement::Result SCD4X::handleMeasurement() {
  if (!periodic_started_) {
    return {.wait = {}, .error = ErrorResult(type(), "Not started")};
  }

  if (!isDeviceConnected(i2c_address_)) {
    return {.wait = {},
            .error = ErrorResult(type(), missingI2CDeviceError(i2c_address_))};
  }

  if (hasCachedMeasurement()) {
    return {.wait = {}};
  }

  const auto wait = waitUntilNextMeasurementCheck();
  if (wait != std::chrono::nanoseconds::zero()) {
    return {.wait = wait};
  }

  bool data_ready = false;
  int16_t error = driver_.getDataReadyStatus(data_ready);
  if (error != 0) {
    return {.wait = {}, .error = ErrorResult(type(), data_ready_status_error_)};
  }

  if (!data_ready) {
    next_measurement_check_time_ =
        std::chrono::steady_clock::now() + kPollInterval;
    return {.wait = kPollInterval};
  }

  error = driver_.readMeasurement(last_co2_ppm_, last_temperature_c_,
                                  last_humidity_rh_);
  if (error != 0) {
    return {.wait = {}, .error = ErrorResult(type(), read_measurement_error_)};
  }

  measurement_ready_ = true;
  measurement_ready_time_ = std::chrono::steady_clock::now();
  return {.wait = {}};
}

capabilities::GetValues::Result SCD4X::getValues() {
  if (!measurement_ready_) {
    return {.values = {}, .error = ErrorResult(type(), get_values_error_)};
  }

  capabilities::GetValues::Result result;
  result.values.push_back(utils::ValueUnit(static_cast<float>(last_co2_ppm_),
                                           co2_data_point_type_));

  if (temperature_data_point_type_.isValid()) {
    result.values.push_back(
        utils::ValueUnit(last_temperature_c_, temperature_data_point_type_));
  }

  if (humidity_data_point_type_.isValid()) {
    result.values.push_back(
        utils::ValueUnit(last_humidity_rh_, humidity_data_point_type_));
  }
  // TRACEF("CO2: %u, °C: %f, %RH: %f\r\n", last_co2_ppm_, last_temperature_c_,
  //        last_humidity_rh_);

  return result;
}

std::shared_ptr<Peripheral> SCD4X::factory(const ServiceGetters& services,
                                           const JsonObjectConst& parameters) {
  return std::make_shared<SCD4X>(parameters);
}

bool SCD4X::registered_ = PeripheralFactory::registerFactory(type(), factory);

bool SCD4X::capability_get_values_ =
    capabilities::GetValues::registerType(type());

bool SCD4X::capability_start_measurement_ =
    capabilities::StartMeasurement::registerType(type());

const __FlashStringHelper* SCD4X::data_ready_status_error_ =
    FPSTR("Failed to read data ready status");
const __FlashStringHelper* SCD4X::read_measurement_error_ =
    FPSTR("Failed to read measurement");
const __FlashStringHelper* SCD4X::co2_data_point_type_key_ =
    FPSTR("co2_data_point_type");

}  // namespace scd4x
}  // namespace peripherals
}  // namespace peripheral
}  // namespace inamata

#endif
