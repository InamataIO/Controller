#include "digital_in.h"

#include "peripheral/peripheral_factory.h"

namespace inamata {
namespace peripheral {
namespace peripherals {
namespace digital_in {

DigitalIn::DigitalIn(const JsonObjectConst& parameters) {
  // Get the pin # for the GPIO output and validate data. Invalidate on error
  int pin = toPin(parameters[pin_key_]);
  if (pin < 0) {
    setInvalid(pin_key_error_);
    return;
  }
  pin_ = pin;

  // Get the data point type for setting the pin state
  data_point_type_ = utils::UUID(parameters[data_point_type_key_]);
  if (!data_point_type_.isValid()) {
    setInvalid(data_point_type_key_error_);
    return;
  }

  // Set up the input pin
  JsonVariantConst input_type = parameters[input_type_key_];
  if (!input_type.is<const char*>()) {
    setInvalid(input_type_key_error_);
    return;
  }
  if (input_type == F("floating")) {
    pinMode(pin_, INPUT);
  } else if (input_type == F("pullup")) {
    pinMode(pin_, INPUT_PULLUP);
  } else if (input_type == F("pulldown")) {
#ifdef ESP32
    pinMode(pin_, INPUT_PULLDOWN);
#else
    pinMode(pin_, INPUT_PULLDOWN_16);
#endif
  } else {
    setInvalid(input_type_key_error_);
    return;
  }

  // Set if it is an active low peripheral. Default is active high
  JsonVariantConst active_low = parameters[active_low_key_];
  if (active_low.is<bool>() == active_low.isNull()) {
    setInvalid(active_low_key_error_);
    return;
  }
  // If null, returns false. If bool, returns true or false
  if (active_low.as<bool>()) {
    active_low_ = true;
  }
}

const String& DigitalIn::getType() const { return type(); }

const String& DigitalIn::type() {
  static const String name{"DigitalIn"};
  return name;
}

capabilities::GetValues::Result DigitalIn::getValues() {
  bool value = bool(digitalRead(pin_));
  if (active_low_) {
    value = !value;
  }
  return {.values = {utils::ValueUnit{.value = static_cast<float>(value),
                                      .data_point_type = data_point_type_}}};
}

std::shared_ptr<Peripheral> DigitalIn::factory(
    const ServiceGetters& services, const JsonObjectConst& parameters) {
  return std::make_shared<DigitalIn>(parameters);
}

bool DigitalIn::registered_ =
    PeripheralFactory::registerFactory(type(), factory);

bool DigitalIn::capability_get_values_ =
    capabilities::GetValues::registerType(type());

const __FlashStringHelper* DigitalIn::input_type_key_ = FPSTR("input_type");
const __FlashStringHelper* DigitalIn::input_type_key_error_ =
    FPSTR("Missing property: input_type (str)");

}  // namespace digital_in
}  // namespace peripherals
}  // namespace peripheral
}  // namespace inamata
