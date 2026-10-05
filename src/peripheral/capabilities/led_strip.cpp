#include "led_strip.h"

namespace inamata {
namespace peripheral {
namespace capabilities {

bool LedStrip::registerType(const String& type) {
  getSupportedTypes().push_back(type);
  return true;
}

bool LedStrip::isSupported(const String& type) {
  const std::vector<String>& types = getSupportedTypes();
  return std::binary_search(types.begin(), types.end(), type);
}

const std::vector<String>& LedStrip::getTypes() { return getSupportedTypes(); }

String LedStrip::invalidTypeError(const utils::UUID& uuid,
                                  std::shared_ptr<Peripheral> peripheral) {
  String error("LedStrip capability not supported: ");
  error += uuid.toString();
  error += String(" is a ");
  error += peripheral->getType();
  return error;
}

std::vector<String>& LedStrip::getSupportedTypes() {
  static std::vector<String> supported_types;
  return supported_types;
}

}  // namespace capabilities
}  // namespace peripheral
}  // namespace inamata
