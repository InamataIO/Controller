#include "math_action.h"

namespace inamata {
namespace lac {

const String& MathAction::type() {
  static const String name{"Math"};
  return name;
}

void MathAction::populateConfig(const JsonObjectConst& parameters,
                                Config& config) {
  JsonVariantConst query = parameters[Action::query_key_];
  if (query.is<const char*>()) {
    config.query = query.as<const char*>();
  }
}

}  // namespace lac
}  // namespace inamata