#ifndef MINIMAL_BUILD
#include "gp2y.h"

#include <GP2YDustSensor.h>

#include <cmath>

#include "managers/logging.h"
#include "peripheral/peripheral_factory.h"

namespace inamata {
namespace peripheral {
namespace peripherals {
namespace gp2y {

GP2Y::GP2Y(const JsonObjectConst& parameters) {
  data_point_type_ = utils::UUID(parameters[data_point_type_key_]);
  if (!data_point_type_.isValid()) {
    setInvalid(data_point_type_key_error_);
    return;
  }

  led_pin = toPin(parameters[led_pin_key_]);
  if (led_pin < 0) {
    setInvalid(led_pin_key_error_);
    return;
  }

  analog_pin = toPin(parameters[pin_key_]);
  if (analog_pin < 0) {
    setInvalid(pin_key_error_);
    return;
  }

  JsonVariantConst sensor_variant = parameters[variant_key_];
  GP2YDustSensorType sensor_type = GP2YDustSensorType::GP2Y1010AU0F;
  if (!sensor_variant.isNull()) {
    if (!sensor_variant.is<const char*>()) {
      setInvalid(variant_key_error_);
      return;
    }
    const char* variant = sensor_variant.as<const char*>();
    if (strcmp(variant, "GP2Y1010AU0F") == 0) {
      sensor_type = GP2YDustSensorType::GP2Y1010AU0F;
    } else if (strcmp(variant, "GP2Y1014AU0F") == 0) {
      sensor_type = GP2YDustSensorType::GP2Y1014AU0F;
    } else {
      setInvalid(variant_key_error_);
      return;
    }
  }

  sensor_ = std::make_unique<GP2YDustSensor>(sensor_type,
                                             static_cast<uint8_t>(led_pin),
                                             static_cast<uint8_t>(analog_pin));
  pinMode(led_pin, OUTPUT);
  pinMode(analog_pin, INPUT);
  sensor_->begin();
}

const String& GP2Y::getType() const { return type(); }

const String& GP2Y::type() {
  static const String name{"GP2Y"};
  return name;
}

capabilities::GetValues::Result GP2Y::getValues() {
  const auto now = std::chrono::steady_clock::now();
  if (!std::isnan(last_density_) && now - last_read_time_ < kCacheDuration_) {
    return {.values = {utils::ValueUnit(last_density_, data_point_type_)},
            .error = ErrorResult()};
  }

  // Wait 200 ms for 20 measurements. Fewer readings lead to the calibration
  // using a too low min and result in unreasonably high readings
  sensor_->getDustDensity(20);
  last_density_ = sensor_->getRunningAverage();
  last_read_time_ = now;
  calibration_read_count_++;

  return {.values = {utils::ValueUnit(last_density_, data_point_type_)},
          .error = ErrorResult()};
}

capabilities::Calibrate::Result GP2Y::startCalibration(
    const JsonObjectConst& parameters) {
  return handleCalibration();
}

capabilities::Calibrate::Result GP2Y::handleCalibration() {
  if (calibration_read_count_ < kBaselineCandidateMinReadings) {
    // Not enough dust density readings taken yet, check again later
    return {.wait = std::chrono::seconds(1), .error = ErrorResult()};
  }

  const float baseline_candidate = sensor_->getBaselineCandidate();
  sensor_->setBaseline(baseline_candidate);
  calibration_read_count_ = 0;

  return {.wait = std::chrono::nanoseconds::zero(), .error = ErrorResult()};
}

std::shared_ptr<Peripheral> GP2Y::factory(const ServiceGetters& services,
                                          const JsonObjectConst& parameters) {
  return std::make_shared<GP2Y>(parameters);
}

bool GP2Y::registered_ = PeripheralFactory::registerFactory(type(), factory);

bool GP2Y::capability_get_values_ =
    capabilities::GetValues::registerType(type());

bool GP2Y::capability_calibrate_ =
    capabilities::Calibrate::registerType(type());

const __FlashStringHelper* GP2Y::led_pin_key_ = FPSTR("led_pin");
const __FlashStringHelper* GP2Y::led_pin_key_error_ =
    FPSTR("Missing property: led_pin (unsigned int)");

}  // namespace gp2y
}  // namespace peripherals
}  // namespace peripheral
}  // namespace inamata

#endif