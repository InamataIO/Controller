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

String Peripheral::peripheralNotFoundError(const utils::UUID& uuid) {
  String error(peripheral_not_found_error_);
  error += uuid.toString();
  return error;
}

String Peripheral::notAValidError(const utils::UUID& uuid, const String& type) {
  return uuid.toString() + F(" is not a valid ") + type;
}

void Peripheral::setInvalid() { valid_ = false; }

void Peripheral::setInvalid(const String& error_message) {
  valid_ = false;
  error_message_ = error_message;
}

utils::UUID Peripheral::getDataPointType(const JsonObjectConst& parameters) {
  JsonVariantConst dpt_variant = parameters[data_point_type_short_key_];
  if (!dpt_variant.isNull()) {
    return utils::UUID(dpt_variant);
  }
  dpt_variant = parameters[data_point_type_key_];
  if (!dpt_variant.isNull()) {
    return utils::UUID(dpt_variant);
  }
  return utils::UUID(nullptr);
}

const __FlashStringHelper* Peripheral::uuid_key_ = FPSTR("uuid");
const __FlashStringHelper* Peripheral::uuid_key_error_ =
    FPSTR("Missing property: uuid (uuid)");
const __FlashStringHelper* Peripheral::peripheral_not_found_error_ =
    FPSTR("Could not find peripheral: ");

const __FlashStringHelper* Peripheral::data_point_type_short_key_ =
    FPSTR("dpt");
const __FlashStringHelper* Peripheral::data_point_type_key_ =
    FPSTR("data_point_type");
const __FlashStringHelper* Peripheral::data_point_type_key_error_ =
    FPSTR("Missing property: data_point_type OR dpt (UUID)");
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

const __FlashStringHelper* Peripheral::variant_key_ = FPSTR("variant");
const __FlashStringHelper* Peripheral::variant_key_error_ =
    FPSTR("Missing property: variant (str)");

const __FlashStringHelper* Peripheral::rx_key_ = FPSTR("rx");
const __FlashStringHelper* Peripheral::tx_key_ = FPSTR("tx");
const __FlashStringHelper* Peripheral::dere_key_ = FPSTR("dere");
const __FlashStringHelper* Peripheral::baud_rate_key_ = FPSTR("baud_rate");
const __FlashStringHelper* Peripheral::config_key_ = FPSTR("config");
const __FlashStringHelper* Peripheral::modbus_client_adapter_key_ =
    FPSTR("adapter");
const __FlashStringHelper* Peripheral::config_error_ = FPSTR("Invalid config");

}  // namespace peripheral
}  // namespace inamata