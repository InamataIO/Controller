#include "analog.h"

namespace inamata {
namespace peripheral {
namespace peripherals {

String Analog::parseParameters(const JsonObjectConst& parameters) {
  // Use both or either voltage and unit as analog readings
  voltage_data_point_type_ =
      utils::UUID(parameters[Analog::voltage_data_point_type_key_]);
  percent_data_point_type_ =
      utils::UUID(parameters[Analog::percent_data_point_type_key_]);
  bool success = parseConvertToUnit(parameters);
  if (!success) {
    return "Failed parsing convert to unit";
  }
  if (!voltage_data_point_type_.isValid() &&
      !percent_data_point_type_.isValid() && !unit_data_point_type_.isValid()) {
    return no_data_point_type_key_error_;
  }
  return String();
}

bool Analog::parseConvertToUnit(const JsonObjectConst& parameters) {
  utils::UUID unit_data_point_type =
      utils::UUID(parameters[unit_data_point_type_key_]);
  if (!unit_data_point_type.isValid()) {
    return true;
  }

  // Do a linear conversion from the voltage to a different unit
  JsonVariantConst min_v = parameters[min_v_key_];
  if (!min_v.is<float>()) {
    return false;
  }
  JsonVariantConst max_v = parameters[max_v_key_];
  if (!max_v.is<float>()) {
    return false;
  }
  JsonVariantConst min_unit = parameters[min_unit_key_];
  if (!min_unit.is<float>()) {
    return false;
  }
  JsonVariantConst max_unit = parameters[max_unit_key_];
  if (!max_unit.is<float>()) {
    return false;
  }
  JsonVariantConst limit_unit = parameters[limit_unit_key_];
  if (limit_unit.is<bool>()) {
    limit_unit_ = limit_unit;
  }
  if (min_v == max_v) {
    return false;
  }

  // Formula taken from SO:
  // https://stackoverflow.com/questions/5731863/mapping-a-numeric-range-onto-another
  v_to_unit_slope_ = 1.0 * (max_unit.as<float>() - min_unit.as<float>()) /
                     (max_v.as<float>() - min_v.as<float>());
  min_v_ = min_v;
  min_unit_ = min_unit;
  max_unit_ = max_unit;
  unit_data_point_type_ = unit_data_point_type;

  return true;
}

const char Analog::min_v_key_[] = "min_v";
const char Analog::max_v_key_[] = "max_v";
const char Analog::min_unit_key_[] = "min_unit";
const char Analog::max_unit_key_[] = "max_unit";
const char Analog::limit_unit_key_[] = "limit_unit";

const char Analog::voltage_data_point_type_key_[] = "voltage_data_point_type";
const char Analog::unit_data_point_type_key_[] = "unit_data_point_type";
const char Analog::percent_data_point_type_key_[] = "percent_data_point_type";
const char Analog::no_data_point_type_key_error_[] = "No data point type set";

}  // namespace peripherals
}  // namespace peripheral
}  // namespace inamata