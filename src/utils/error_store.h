#pragma once

#include <Arduino.h>

#include "utils/uuid.h"

namespace inamata {
class ErrorStore {
 public:
  enum class KeyType {
    kArray,
    kBool,
    kFloat,
    kString,
    kUint32t,
    kUUID,
  };

  static String genMissingProperty(String key, KeyType key_type,
                                   bool type_error = false);

  static String genNotAValid(const utils::UUID& uuid, const String& type);

 private:
  static const char* missing_property_prefix_;
  static const char* invalid_type_prefix_;
  static const char* array_type_;
  static const char* bool_type_;
  static const char* float_type_;
  static const char* string_type_;
  static const char* uuid_type_;
  static const char* uint32_t_type_;
  static const char* unknown_type_;
};
}  // namespace inamata
