#ifndef MINIMAL_BUILD
#include "sht2x_sensor.h"

#include <cmath>

#include "peripheral/peripheral_factory.h"
#include "utils/chrono.h"

namespace inamata {
namespace peripheral {
namespace peripherals {
namespace sht2x_sensor {

SHT2xSensor::SHT2xSensor(const JsonObjectConst& parameters)
    : I2CAbstractPeripheral(parameters) {
  if (!isValid()) {
    return;
  }

  temperature_data_point_type_ =
      utils::UUID(parameters[temperature_data_point_type_key_]);
  if (!temperature_data_point_type_.isValid()) {
    setInvalid(temperature_data_point_type_key_error_);
    return;
  }

  humidity_data_point_type_ =
      utils::UUID(parameters[humidity_data_point_type_key_]);
  if (!humidity_data_point_type_.isValid()) {
    setInvalid(humidity_data_point_type_key_error_);
    return;
  }

  const int i2c_address = parseI2CAddress(parameters[i2c_address_key_]);
  if (i2c_address < 0) {
    setInvalid(i2c_address_key_error_);
    return;
  }
  i2c_address_ = i2c_address;

  driver_ = std::make_unique<::SHT2x>(getWire());

  // A probe may be intentionally disconnected at boot. Keep the configured
  // peripheral valid and retry initialization when a measurement is started.
  if (isDeviceConnected(i2c_address_)) {
    initialized_ = driver_->begin();
  }
}

const String& SHT2xSensor::getType() const { return type(); }

const String& SHT2xSensor::type() {
  static const String name{"SHT2x"};
  return name;
}

bool SHT2xSensor::isInitialized() const { return initialized_; }

bool SHT2xSensor::hasCachedMeasurement() const {
  return measurement_state_ == MeasurementState::kReady &&
         !utils::hasTimedOut(measurement_ready_time_, kCacheDuration);
}

std::chrono::nanoseconds SHT2xSensor::waitUntilNextCheck() const {
  const auto now = std::chrono::steady_clock::now();
  if (now >= next_check_time_) {
    return {};
  }
  return std::chrono::duration_cast<std::chrono::nanoseconds>(next_check_time_ -
                                                              now);
}

capabilities::StartMeasurement::Result SHT2xSensor::measurementError(
    const String& message) {
  measurement_state_ = MeasurementState::kIdle;
  temperature_ = NAN;
  humidity_ = NAN;
  initialized_ = false;
  return {.wait = {}, .error = ErrorResult(type(), message)};
}

capabilities::StartMeasurement::Result SHT2xSensor::startMeasurement(
    const JsonVariantConst& parameters) {
  if (!initialized_) {
    if (!isDeviceConnected(i2c_address_)) {
      return measurementError(missingI2CDeviceError(i2c_address_));
    }
    if (!driver_->begin()) {
      return measurementError("Failed to initialize SHT2x sensor");
    }
    initialized_ = true;
  }

  if (hasCachedMeasurement()) {
    return {.wait = {}};
  }
  if (measurement_state_ == MeasurementState::kTemperature ||
      measurement_state_ == MeasurementState::kHumidity) {
    return {.wait = waitUntilNextCheck()};
  }

  measurement_state_ = MeasurementState::kIdle;
  temperature_ = NAN;
  humidity_ = NAN;
  if (!driver_->requestTemperature()) {
    return measurementError("Failed to request temperature");
  }
  measurement_state_ = MeasurementState::kTemperature;
  next_check_time_ = std::chrono::steady_clock::now() + kReadyCheckInterval;
  return {.wait = kReadyCheckInterval};
}

capabilities::StartMeasurement::Result SHT2xSensor::handleMeasurement() {
  if (hasCachedMeasurement()) {
    return {.wait = {}};
  }
  if (measurement_state_ == MeasurementState::kIdle ||
      measurement_state_ == MeasurementState::kReady) {
    return {.wait = {},
            .error = ErrorResult(type(), "Measurement not started")};
  }

  const auto wait = waitUntilNextCheck();
  if (wait != std::chrono::nanoseconds::zero()) {
    return {.wait = wait};
  }

  if (measurement_state_ == MeasurementState::kTemperature) {
    if (!driver_->reqTempReady()) {
      next_check_time_ = std::chrono::steady_clock::now() + kReadyCheckInterval;
      return {.wait = kReadyCheckInterval};
    }
    if (!driver_->readTemperature()) {
      return measurementError("Failed to read temperature");
    }
    temperature_ = driver_->getTemperature();
    if (!std::isfinite(temperature_)) {
      return measurementError("Invalid temperature");
    }
    if (!driver_->requestHumidity()) {
      return measurementError("Failed to request humidity");
    }
    measurement_state_ = MeasurementState::kHumidity;
    next_check_time_ = std::chrono::steady_clock::now() + kReadyCheckInterval;
    return {.wait = kReadyCheckInterval};
  }

  if (!driver_->reqHumReady()) {
    next_check_time_ = std::chrono::steady_clock::now() + kReadyCheckInterval;
    return {.wait = kReadyCheckInterval};
  }
  if (!driver_->readHumidity()) {
    return measurementError("Failed to read humidity");
  }
  humidity_ = driver_->getHumidity();
  if (!std::isfinite(humidity_)) {
    return measurementError("Invalid humidity");
  }
  measurement_state_ = MeasurementState::kReady;
  measurement_ready_time_ = std::chrono::steady_clock::now();
  TRACEF("Humidity: %f %%RH, Temp: %f C", humidity_, temperature_);
  return {.wait = {}};
}

capabilities::GetValues::Result SHT2xSensor::getValues() {
  if (!hasCachedMeasurement()) {
    return {.values = {}, .error = ErrorResult(type(), get_values_error_)};
  }

  capabilities::GetValues::Result result;
  result.values.push_back(
      utils::ValueUnit(temperature_, temperature_data_point_type_));
  result.values.push_back(
      utils::ValueUnit(humidity_, humidity_data_point_type_));
  return result;
}

std::shared_ptr<Peripheral> SHT2xSensor::factory(
    const ServiceGetters& services, const JsonObjectConst& parameters) {
  return std::make_shared<SHT2xSensor>(parameters);
}

bool SHT2xSensor::registered_ =
    PeripheralFactory::registerFactory(type(), factory);
bool SHT2xSensor::capability_get_values_ =
    capabilities::GetValues::registerType(type());
bool SHT2xSensor::capability_start_measurement_ =
    capabilities::StartMeasurement::registerType(type());

}  // namespace sht2x_sensor
}  // namespace peripherals
}  // namespace peripheral
}  // namespace inamata
#endif
