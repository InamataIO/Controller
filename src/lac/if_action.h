#pragma once

#include <ArduinoJson.h>

#include "lac/action.h"

namespace inamata {
namespace lac {

class IfAction {
 public:
  struct Config : public Action::Config {
    virtual ~Config() = default;
    String query;
    uint8_t action_count;
  };

  static const String& type();

  static void populateConfig(const JsonObjectConst& parameters, Config& input,
                             std::vector<Action>& actions);
};

}  // namespace lac
}  // namespace inamata