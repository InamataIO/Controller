#include "analog_in.h"

#include "peripheral/peripheral_factory.h"
#include "peripheral/peripherals/analog.h"

namespace inamata {
namespace peripheral {
namespace peripherals {
namespace analog_in {

AnalogIn::AnalogIn(const JsonObjectConst& parameters) {
  // Get the pin # for the GPIO output and validate data. Invalidate on error
  int pin = toPin(parameters[pin_key_]);
  if (pin < 0) {
    setInvalid(pin_key_error_);
    return;
  }
  pin_ = pin;

  // Check if the pin supports analog input (ADC)
  if (valid_pins_.size()) {
    // Skip valid pins check if first value is -1
    if (valid_pins_[0] != -1) {
      if (std::find(valid_pins_.begin(), valid_pins_.end(), pin_) ==
          valid_pins_.end()) {
        setInvalid(invalid_pin_error_);
        return;
      }
    }
  } else {
    setInvalid(invalid_pin_error_);
    return;
  }

  String error = parseParameters(parameters);
  if (!error.isEmpty()) {
    setInvalid(error);
  }
}

const String& AnalogIn::getType() const { return type(); }

const String& AnalogIn::type() {
  static const String name{"AnalogIn"};
  return name;
}

capabilities::GetValues::Result AnalogIn::getValues() {
  std::vector<utils::ValueUnit> values;
  const uint16_t value = analogRead(pin_);
  const float voltage = value * 3.3 / 4096.0;

  if (voltage_data_point_type_.isValid()) {
    values.push_back({utils::ValueUnit{
        .value = voltage, .data_point_type = voltage_data_point_type_}});
  }
  if (percent_data_point_type_.isValid()) {
    const float percentage = value / 4096.0;
    values.push_back({utils::ValueUnit{
        .value = percentage, .data_point_type = percent_data_point_type_}});
  }
  if (unit_data_point_type_.isValid()) {
    float unit_value = min_unit_ + v_to_unit_slope_ * (voltage - min_v_);
    if (limit_unit_) {
      // Constrain mapped value between min and max unit
      if (min_unit_ < max_unit_) {
        unit_value = std::max(min_unit_, std::min(unit_value, max_unit_));
      } else {
        unit_value = std::max(max_unit_, std::min(unit_value, min_unit_));
      }
    }
    values.push_back({utils::ValueUnit{
        .value = unit_value, .data_point_type = unit_data_point_type_}});
  }

  return {.values = values, .error = ErrorResult()};
}

std::shared_ptr<Peripheral> AnalogIn::factory(
    const ServiceGetters& services, const JsonObjectConst& parameters) {
  return std::make_shared<AnalogIn>(parameters);
}

bool AnalogIn::registered_ =
    PeripheralFactory::registerFactory(type(), factory);

bool AnalogIn::capability_get_values_ =
    capabilities::GetValues::registerType(type());

const std::array<uint8_t, 8> AnalogIn::valid_pins_ = {
    32, 33, 34, 35, 36, 37, 38, 39,
};
const __FlashStringHelper* AnalogIn::invalid_pin_error_ =
    FPSTR("Pin # not valid (only ADC1: 32 - 39)");

}  // namespace analog_in
}  // namespace peripherals
}  // namespace peripheral
}  // namespace inamata