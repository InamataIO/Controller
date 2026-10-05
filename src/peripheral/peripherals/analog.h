#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>

#include "utils/uuid.h"

namespace inamata {
namespace peripheral {
namespace peripherals {

class Analog {
 public:
  static const char min_v_key_[];
  static const char max_v_key_[];
  static const char min_unit_key_[];
  static const char max_unit_key_[];
  static const char limit_unit_key_[];

  static const char voltage_data_point_type_key_[];
  static const char unit_data_point_type_key_[];
  static const char percent_data_point_type_key_[];
  /// Error if neither unit nor voltage data point types are set
  static const char no_data_point_type_key_error_[];

 protected:
  String parseParameters(const JsonObjectConst& parameters);

  /**
   * \return True on no action or success. False on failure
   */
  bool parseConvertToUnit(const JsonObjectConst& parameters);

  /// Data point type for the reading as voltage
  utils::UUID voltage_data_point_type_{nullptr};
  utils::UUID unit_data_point_type_{nullptr};
  utils::UUID percent_data_point_type_{nullptr};

  float min_v_ = NAN;
  float min_unit_ = NAN;
  float max_unit_ = NAN;
  float v_to_unit_slope_ = NAN;
  bool limit_unit_ = true;
};

}  // namespace peripherals
}  // namespace peripheral
}  // namespace inamata