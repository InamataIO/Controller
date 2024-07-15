#pragma once

#include <ArduinoJson.h>

#include "lac/action.h"

namespace inamata {
namespace lac {

class MathAction {
 public:
  struct Config : public Action::Config {
    virtual ~Config() = default;
    String query;
  };

  static const String& type();

  static void populateConfig(const JsonObjectConst& parameters, Config& input);
};

}  // namespace lac
}  // namespace inamata