#include "set_value.h"

namespace inamata {
namespace peripheral {
namespace capabilities {

bool SetValue::registerType(const String& type) {
  getSupportedTypes().push_back(type);
  return true;
}

bool SetValue::isSupported(const String& type) {
  const std::vector<String>& types = getSupportedTypes();
  return std::binary_search(types.begin(), types.end(), type);
}

const std::vector<String>& SetValue::getTypes() { return getSupportedTypes(); }

String SetValue::invalidTypeError(const utils::UUID& uuid,
                                  std::shared_ptr<Peripheral> peripheral) {
  String error("SetValue capability not supported: ");
  error += uuid.toString();
  error += " is a ";
  error += peripheral->getType();
  return error;
}

std::vector<String>& SetValue::getSupportedTypes() {
  static std::vector<String> supported_types;
  return supported_types;
}

}  // namespace capabilities
}  // namespace peripheral
}  // namespace inamata