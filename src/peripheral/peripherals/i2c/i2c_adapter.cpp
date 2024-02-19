#ifdef ESP32

#include "i2c_adapter.h"

#include "peripheral/peripheral_factory.h"

namespace inamata {
namespace peripheral {
namespace peripherals {
namespace util {

bool I2CAdapter::wire_taken = false;
bool I2CAdapter::wire1_taken = false;

I2CAdapter::I2CAdapter(const ServiceGetters& services,
                       const JsonObjectConst& parameter) {
  web_socket_ = services.getWebSocket();
  if (web_socket_ == nullptr) {
    setInvalid(services.web_socket_nullptr_error_);
    return;
  }

  int clock_pin = toPin(parameter[scl_key_]);
  if (clock_pin < 0) {
    setInvalid(scl_key_error_);
    return;
  }

  int data_pin = parameter[sda_key_];
  if (data_pin < 0) {
    setInvalid(sda_key_error_);
    return;
  }

  if (!wire_taken) {
    taken_variable = &wire_taken;
    wire_ = &Wire;
  } else if (!wire1_taken) {
    taken_variable = &wire1_taken;
    wire_ = &Wire1;
  } else {
    web_socket_->sendError(type(), F("Both wires already taken"));
    setInvalid();
    return;
  }

  *taken_variable = true;
  wire_->begin(data_pin, clock_pin, 0);
}

I2CAdapter::~I2CAdapter() {
  if (taken_variable) {
    *taken_variable = false;
  }
}

const String& I2CAdapter::getType() const { return type(); }

const String& I2CAdapter::type() {
  static const String name{"I2CAdapter"};
  return name;
}

TwoWire* I2CAdapter::getWire() { return wire_; }

std::shared_ptr<Peripheral> I2CAdapter::factory(
    const ServiceGetters& services, const JsonObjectConst& parameter) {
  return std::make_shared<I2CAdapter>(services, parameter);
}

bool I2CAdapter::registered_ =
    PeripheralFactory::registerFactory(type(), factory);

const __FlashStringHelper* I2CAdapter::scl_key_ = FPSTR("scl");
const __FlashStringHelper* I2CAdapter::scl_key_error_ =
    FPSTR("Missing property: scl (unsigned int)");
const __FlashStringHelper* I2CAdapter::sda_key_ = FPSTR("sda");
const __FlashStringHelper* I2CAdapter::sda_key_error_ =
    FPSTR("Missing property: sda (unsigned int)");

}  // namespace util
}  // namespace peripherals
}  // namespace peripheral
}  // namespace inamata

#endif
