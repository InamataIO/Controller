#include "digital_out.h"

#include "peripheral/peripheral_factory.h"

namespace inamata {
namespace peripheral {
namespace peripherals {
namespace digital_out {

DigitalOut::DigitalOut(const ServiceGetters& services,
                       const JsonObjectConst& parameters) {
  web_socket_ = services.getWebSocket();
  if (web_socket_ == nullptr) {
    setInvalid(ServiceGetters::web_socket_nullptr_error_);
    return;
  }

  // Get the pin # for the GPIO output and validate data. Invalidate on error
  int pin = toPin(parameters[pin_key_]);
  if (pin < 0) {
    setInvalid(pin_key_error_);
    return;
  }
  pin_ = pin;

  data_point_type_ = getDataPointType(parameters);
  if (!data_point_type_.isValid()) {
    setInvalid(data_point_type_key_error_);
    return;
  }

  // Check if the initial state was set, if yes, if it has the correct type
  JsonVariantConst initial_state = parameters[initial_state_key_];
  if (initial_state.is<bool>() == initial_state.isNull()) {
    setInvalid(initial_state_key_error_);
    return;
  }

  // Set if it is an active low peripheral. Default is active high
  JsonVariantConst active_low = parameters[active_low_key_];
  if (active_low.is<bool>() == active_low.isNull()) {
    setInvalid(active_low_key_error_);
    return;
  }
  if (active_low.as<bool>()) {
    active_low_ = true;
  }

  // Setup pin to be control the GPIO state
  pinMode(pin_, OUTPUT);
  // If specified set the initial state
  if (!initial_state.isNull()) {
    state_ = initial_state;
    setOutput();
  }
}

const String& DigitalOut::getType() const { return type(); }

const String& DigitalOut::type() {
  static const String name{"DigitalOut"};
  return name;
}

void DigitalOut::setValue(utils::ValueUnit value_unit) {
  if (value_unit.data_point_type != data_point_type_) {
    web_socket_->sendError(type(),
                           value_unit.sourceUnitError(data_point_type_));
    return;
  }

  // Limit the value between 0 and 1 and then round to the nearest integer.
  // Finally, set the pin value
  float clamped_value = std::fmax(0, std::fmin(value_unit.value, 1));
  state_ = std::lround(clamped_value);

  setOutput();
}

void DigitalOut::setOverride(bool state) {
  override_state_ = state;
  setOutput();
}

void DigitalOut::clearOverride() {
  override_state_ = -1;
  setOutput();
}

bool DigitalOut::getState() { return state_; }

void DigitalOut::setOutput() {
  bool state = state_;
  if (override_state_ != -1) {
    state = override_state_;
  }

  // Respect active low logic. If true, invert state
  if (active_low_) {
    digitalWrite(pin_, !state);
  } else {
    digitalWrite(pin_, state);
  }
}

std::shared_ptr<Peripheral> DigitalOut::factory(
    const ServiceGetters& services, const JsonObjectConst& parameters) {
  return std::make_shared<DigitalOut>(services, parameters);
}

bool DigitalOut::registered_ =
    PeripheralFactory::registerFactory(type(), factory);

bool DigitalOut::capability_set_value_ =
    capabilities::SetValue::registerType(type());

const __FlashStringHelper* DigitalOut::initial_state_key_ =
    FPSTR("initial_state");
const __FlashStringHelper* DigitalOut::initial_state_key_error_ =
    FPSTR("Wrong property: initial_state (bool)");

}  // namespace digital_out
}  // namespace peripherals
}  // namespace peripheral
}  // namespace inamata