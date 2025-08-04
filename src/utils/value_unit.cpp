#include "value_unit.h"

namespace inamata {
namespace utils {

ValueUnit::ValueUnit(float value, UUID data_point_type)
    : value(value), data_point_type(data_point_type) {}

const String ValueUnit::sourceUnitError(const UUID& other_data_point_type) {
  return mismatchUnitError(other_data_point_type, data_point_type);
}

const String ValueUnit::targetUnitError(const UUID& other_data_point_type) {
  return mismatchUnitError(data_point_type, other_data_point_type);
}

const char* ValueUnit::value_key = "value";
const char* ValueUnit::value_key_error = "Missing property: value (String)";

const char* ValueUnit::data_point_type_key = "data_point_type";
const char* ValueUnit::fixed_data_point_type_key = "fdpt_id";
const char* ValueUnit::data_point_type_key_error =
    "Missing property: data_point_type (String)";

const char* ValueUnit::data_points_key = "data_points";

String ValueUnit::mismatchUnitError(const UUID& expected_data_point_type,
                                    const UUID& received_data_point_type) {
  String error(F("Mismatching unit. Got: "));
  error += received_data_point_type.toString();
  error += F(" instead of ");
  error += expected_data_point_type.toString();
  return error;
}

}  // namespace utils
}  // namespace inamata