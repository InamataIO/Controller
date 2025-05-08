#include "error_store.h"

namespace inamata {

String ErrorStore::genMissingProperty(String key, KeyType key_type,
                                      bool type_error) {
  const char* key_type_string;
  switch (key_type) {
    case KeyType::kArray:
      key_type_string = array_type_;
      break;
    case KeyType::kBool:
      key_type_string = bool_type_;
      break;
    case KeyType::kFloat:
      key_type_string = float_type_;
      break;
    case KeyType::kString:
      key_type_string = string_type_;
      break;
    case KeyType::kUint32t:
      key_type_string = uint32_t_type_;
      break;
    case KeyType::kUUID:
      key_type_string = uuid_type_;
      break;
    default:
      key_type_string = unknown_type_;
      break;
  }

  // The length A consists of the prefix + padding + terminator
  // The length B is the length of the key
  // The lenght C is the length of the key type
  size_t length = 2 + 2 + key.length() + strlen(key_type_string);
  if (type_error) {
    // Invalid type: some_key (some_type)
    // |------------>|------>|>|--------|>
    // A-------------B-------A-C--------A-
    length += strlen(invalid_type_prefix_);
  } else {
    // Missing property: some_key (some_type)
    // |------------>|------>|>|--------|>
    // A-------------B-------A-C--------A-
    length += strlen(missing_property_prefix_);
  }

  String error;
  error.reserve(length);
  if (type_error) {
    error = invalid_type_prefix_;
  } else {
    error = missing_property_prefix_;
  }
  error += key + " (" + key_type_string + ")";
  return error;
}

String ErrorStore::genNotAValid(const utils::UUID& uuid, const String& type) {
  return uuid.toString() + " not a valid " + type;
}

const char* ErrorStore::missing_property_prefix_ = "Missing property: ";
const char* ErrorStore::invalid_type_prefix_ = "Invalid type: ";
const char* ErrorStore::array_type_ = "array";
const char* ErrorStore::bool_type_ = "bool";
const char* ErrorStore::float_type_ = "float";
const char* ErrorStore::string_type_ = "string";
const char* ErrorStore::uint32_t_type_ = "uint32_t";
const char* ErrorStore::uuid_type_ = "uuid";
const char* ErrorStore::unknown_type_ = "unknown";

}  // namespace inamata