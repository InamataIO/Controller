#include "lac/action.h"

namespace inamata {
namespace lac {

const __FlashStringHelper* ActionOut::telemetry_key_ = FPSTR("telemetry");
const __FlashStringHelper* ActionOut::variable_key_ = FPSTR("var");

void Action::parseOuts(JsonArrayConst out_configs, Action& action) {
  action.action_outs.reserve(out_configs.size());
  size_t current_target_num = 0;

  TRACEF("Out configs %d\r\n", out_configs.size());
  // Loop through target in JSON config and create parsed structs
  for (const JsonObjectConst& out_config : out_configs) {
    JsonVariantConst type = out_config["type"];
    TRACEF("Type %s\r\n", type.as<const char*>());

    if (type == ActionOut::telemetry_key_) {
      // Telemetry
      action.action_outs.push_back({.type = ActionOut::Type::Telemetry});

    } else if (type == ActionOut::variable_key_) {
      // Variable
      action.action_outs.push_back(
          {.type = ActionOut::Type::Variable,
           .name = out_config["name"].as<const char*>(),
           .data_point_type = out_config["dpt"]});

    } else {
      TRACEF("Unknown target type: %s", out_config["type"]);
    }
    current_target_num++;
  }
  // Incase unknown outs are found, shrink to free unneeded memory
  action.action_outs.shrink_to_fit();
  TRACEF("Outs parsed %d\r\n", action.action_outs.size());
}

const __FlashStringHelper* Action::query_key_ = FPSTR("query");

}  // namespace lac
}  // namespace inamata