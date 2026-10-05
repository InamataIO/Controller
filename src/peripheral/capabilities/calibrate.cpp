#include "calibrate.h"

namespace inamata {
namespace peripheral {
namespace capabilities {

bool Calibrate::registerType(const String& type) {
  getSupportedTypes().push_back(type);
  return true;
}

bool Calibrate::isSupported(const String& type) {
  const std::vector<String>& types = getSupportedTypes();
  return std::find(types.begin(), types.end(), type) != types.end();
}

const std::vector<String>& Calibrate::getTypes() { return getSupportedTypes(); }

String Calibrate::invalidTypeError(const utils::UUID& uuid,
                                   std::shared_ptr<Peripheral> peripheral) {
  String error("Calibrate capability not supported: ");
  error += uuid.toString();
  error += " is a ";
  error += peripheral->getType();
  return error;
}

std::vector<String>& Calibrate::getSupportedTypes() {
  static std::vector<String> supported_types;
  return supported_types;
}

}  // namespace capabilities
}  // namespace peripheral
}  // namespace inamata
