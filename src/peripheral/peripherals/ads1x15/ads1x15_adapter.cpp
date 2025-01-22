#include "ads1x15_adapter.h"

#include "peripheral/peripheral_factory.h"

namespace inamata {
namespace peripheral {
namespace peripherals {
namespace ads1x15 {

ADS1X15Adapter::ADS1X15Adapter(const JsonObjectConst& parameters)
    : I2CAbstractPeripheral(parameters) {
  if (!isValid()) {
    return;
  }

  // Get and check the I2C address of the ADS1x15 chip
  int i2c_address = parseI2CAddress(parameters[i2c_address_key_]);
  if (i2c_address < 0) {
    setInvalid(i2c_address_key_error_);
    return;
  }
  i2c_address_ = i2c_address;

  // Choose the ADS1x15 variant (ADS1015, ADS1115)
  JsonVariantConst input_type = parameters[variant_key_];
  if (!input_type.is<const char*>()) {
    setInvalid(variant_key_error_);
    return;
  }
  if (input_type == F("ads1015")) {
    driver_ = std::unique_ptr<Adafruit_ADS1X15>(new Adafruit_ADS1015());
  } else if (input_type == F("ads1115")) {
    driver_ = std::unique_ptr<Adafruit_ADS1X15>(new Adafruit_ADS1115());
  } else {
    setInvalid(variant_key_error_);
    return;
  }

  // Do a preliminary check to see if the device is connected to the bus
  if (!isDeviceConnected(i2c_address_)) {
    setInvalid(missingI2CDeviceError(i2c_address_));
    return;
  }

  // Initialize the driver with the correct Wire (I2C) interface
  bool setup_success = driver_->begin(i2c_address_, getWire());
  if (!setup_success) {
    setInvalid(invalid_chip_type_error_);
    return;
  }
}

const String& ADS1X15Adapter::getType() const { return type(); }

const String& ADS1X15Adapter::type() {
  static const String name{"ADS1X15Adapter"};
  return name;
}

capabilities::StartMeasurement::Result ADS1X15Adapter::startADCReading(
    ReadingConfig reading_config) {
  reading_config.started_at = std::chrono::steady_clock::now();
  queued_readings_.push_back(reading_config);
  // If there are no other readings queued, start the ADC conversion
  if (queued_readings_.size() == 1) {
    startNextReading();
  }
  return {.wait = default_wait_time_};
}

std::chrono::nanoseconds ADS1X15Adapter::isFinished(void* caller) {
  // Check if the first reading has expired (not closed properly)
  if (!queued_readings_.empty()) {
    const ReadingConfig& config = queued_readings_.front();
    if (config.started_at + std::chrono::seconds(10) <
        std::chrono::steady_clock::now()) {
      queued_readings_.pop_front();
    }
  }

  // Find where in the queue the caller is
  uint8_t position = 0;
  for (const auto i : queued_readings_) {
    if (i.caller == caller) {
      break;
    }
    position++;
  }
  if (position > queued_readings_.size() || queued_readings_.empty()) {
    return std::chrono::nanoseconds{-1};
  }
  if (position != 0) {
    // The reading of the caller is not currently active so tell them to wait
    return default_wait_time_;
  }
  // The ADC reading of the caller is active. If it's not ready, return wait
  if (driver_->conversionComplete()) {
    return {};
  } else {
    return default_wait_time_;
  }
}

ADS1X15Adapter::Result ADS1X15Adapter::completeReading(void* caller) {
  if (!queued_readings_.size()) {
    return {.voltage = NAN,
            .error = ErrorResult(type(), F("No queued reading"))};
  }
  const ReadingConfig& config = queued_readings_.front();
  if (config.caller != caller) {
    return {.voltage = NAN,
            .error = ErrorResult(type(), F("Mismatched caller"))};
  }
  const uint16_t adc_reading = driver_->getLastConversionResults();
  const float voltage = driver_->computeVolts(adc_reading);
  queued_readings_.pop_front();
  startNextReading();
  return {.voltage = voltage};
}

void ADS1X15Adapter::startNextReading() {
  if (queued_readings_.empty()) {
    return;
  }
  const ReadingConfig& config = queued_readings_.front();
  driver_->setGain(config.gain);
  driver_->startADCReading(config.channel, false);
}

std::shared_ptr<Peripheral> ADS1X15Adapter::factory(
    const ServiceGetters& services, const JsonObjectConst& parameters) {
  return std::make_shared<ADS1X15Adapter>(parameters);
}

bool ADS1X15Adapter::registered_ =
    PeripheralFactory::registerFactory(type(), factory);

const __FlashStringHelper* ADS1X15Adapter::invalid_chip_type_error_ =
    FPSTR("Failed ADS1x15 setup");

}  // namespace ads1x15
}  // namespace peripherals
}  // namespace peripheral
}  // namespace inamata
