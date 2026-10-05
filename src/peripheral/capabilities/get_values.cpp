#include "get_values.h"

namespace inamata {
namespace peripheral {
namespace capabilities {

bool GetValues::registerType(const String& type) {
  getSupportedTypes().push_back(type);
  return true;
}

bool GetValues::isSupported(const String& type) {
  const std::vector<String>& types = getSupportedTypes();
  return std::binary_search(types.begin(), types.end(), type);
}

const std::vector<String>& GetValues::getTypes() { return getSupportedTypes(); }

String GetValues::invalidTypeError(const utils::UUID& uuid,
                                   std::shared_ptr<Peripheral> peripheral) {
  String error("GetValues capability not supported: ");
  error += uuid.toString();
  error += " is a ";
  error += peripheral->getType();
  return error;
}

const char* GetValues::get_values_error_ = "GetValues error";

std::vector<String>& GetValues::getSupportedTypes() {
  static std::vector<String> supported_types;
  return supported_types;
}

}  // namespace capabilities
}  // namespace peripheral
}  // namespace inamata