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
    values.push_back({utils::ValueUnit(voltage, voltage_data_point_type_)});
  }
  if (percent_data_point_type_.isValid()) {
    const float percentage = value / 4096.0;
    values.push_back({utils::ValueUnit(percentage, percent_data_point_type_)});
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
    values.push_back({utils::ValueUnit(unit_value, unit_data_point_type_)});
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

}  // namespace analog_in
}  // namespace peripherals
}  // namespace peripheral
}  // namespace inamata