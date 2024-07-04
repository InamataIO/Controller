#include "peripheral.h"

namespace inamata {
namespace peripheral {

bool Peripheral::isValid() const { return valid_; }

ErrorResult Peripheral::getError() const {
  if (valid_) {
    return ErrorResult();
  } else {
    return ErrorResult(getType(), error_message_);
  }
}

int Peripheral::toPin(JsonVariantConst pin) {
  if (!pin.is<float>()) {
    return -1;
  }
  int pin_number = pin.as<int>();
  if (pin_number < 0 || pin_number > 255) {
    return -1;
  }
  return pin_number;
}

void Peripheral::setInvalid() { valid_ = false; }

void Peripheral::setInvalid(const String& error_message) {
  valid_ = false;
  error_message_ = error_message;
}

String Peripheral::notAValidError(const utils::UUID& uuid, const String& type) {
  return uuid.toString() + F(" is not a valid ") + type;
}

const __FlashStringHelper* Peripheral::data_point_type_key_ =
    FPSTR("data_point_type");
const __FlashStringHelper* Peripheral::data_point_type_key_error_ =
    FPSTR("Missing property: data_point_type (UUID)");
const __FlashStringHelper* Peripheral::temperature_data_point_type_key_ =
    FPSTR("temperature_data_point_type");
const __FlashStringHelper* Peripheral::temperature_data_point_type_key_error_ =
    FPSTR("Missing property: temperature_data_point_type (UUID)");
const __FlashStringHelper* Peripheral::pressure_data_point_type_key_ =
    FPSTR("pressure_data_point_type");
const __FlashStringHelper* Peripheral::pressure_data_point_type_key_error_ =
    FPSTR("Missing property: pressure_data_point_type (UUID)");
const __FlashStringHelper* Peripheral::humidity_data_point_type_key_ =
    FPSTR("humidity_data_point_type");
const __FlashStringHelper* Peripheral::humidity_data_point_type_key_error_ =
    FPSTR("Missing property: humidity_data_point_type (UUID)");

const __FlashStringHelper* Peripheral::pin_key_ = FPSTR("pin");
const __FlashStringHelper* Peripheral::pin_key_error_ =
    FPSTR("Missing property: pin (unsigned int)");
const __FlashStringHelper* Peripheral::active_low_key_ = FPSTR("active_low");
const __FlashStringHelper* Peripheral::active_low_key_error_ =
    FPSTR("Wrong property: active_low (bool)");
const __FlashStringHelper* Peripheral::temperature_c_key_ =
    FPSTR("temperature_c");
const __FlashStringHelper* Peripheral::temperature_c_key_error_ =
    FPSTR("Wrong property: temperature_c (float)");
const __FlashStringHelper* Peripheral::humidity_rh_key_ = FPSTR("humidity_rh");
const __FlashStringHelper* Peripheral::humidity_rh_key_error_ =
    FPSTR("Wrong property: humidity_rh (float)");

}  // namespace peripheral
}  // namespace inamata