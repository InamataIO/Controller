#include "lac/if_action.h"

#include "lac/math_action.h"
#include "tasks/alert_sensor/alert_sensor.h"
#include "tasks/read_sensor/read_sensor.h"
#include "tasks/set_rgb_led/set_rgb_led.h"
#include "tasks/set_value/set_value.h"

namespace inamata {
namespace lac {

const String& IfAction::type() {
  static const String name{"If"};
  return name;
}

void IfAction::populateConfig(const JsonObjectConst& json, Config& config,
                              std::vector<Action>& actions) {
  JsonVariantConst query = json[Action::query_key_];
  if (query.is<const char*>()) {
    config.query = query.as<const char*>();
  }

  JsonArrayConst routine = json["then"].as<JsonArrayConst>();

  for (const JsonObjectConst& action_config : routine) {
    TRACEKJSON("action: ", action_config);
    actions.emplace_back();
    Action& action = actions.back();
    config.action_count++;

    JsonVariantConst action_type = action_config["type"];
    if (action_type == tasks::read_sensor::ReadSensor::type()) {
      // Read sensor task
      action.type = Action::Type::ReadSensor;
      auto input = new tasks::read_sensor::ReadSensor::Input();
      input->local_task = true;
      tasks::read_sensor::ReadSensor::populateInput(action_config["params"],
                                                    *input);
      action.input = std::unique_ptr<tasks::BaseTask::Input>(input);
      Action::parseOuts(action_config["out"], action);

    } else if (action_type == tasks::alert_sensor::AlertSensor::type()) {
      // Alert sensor task
      action.type = Action::Type::AlertSensor;
      auto input = new tasks::alert_sensor::AlertSensor::Input();
      input->local_task = true;
      tasks::alert_sensor::AlertSensor::populateInput(action_config["params"],
                                                      *input);
      action.input = std::unique_ptr<tasks::BaseTask::Input>(input);
      Action::parseOuts(action_config["out"], action);

    } else if (action_config["type"] == tasks::set_value::SetValue::type()) {
      // Set value task
      action.type = Action::Type::SetValue;
      auto input = new tasks::set_value::SetValue::Input();
      input->local_task = true;
      input->value_unit.value = action_config["source"]["value"];
      tasks::set_value::SetValue::populateInput(action_config["params"],
                                                *input);
      action.input = std::unique_ptr<tasks::BaseTask::Input>(input);
      Action::parseOuts(action_config["out"], action);

    } else if (action_type == tasks::set_rgb_led::SetRgbLed::type()) {
      // Set RGB LED task
      action.type = Action::Type::SetRgbLed;
      auto input = new tasks::set_rgb_led::SetRgbLed::Input();
      input->local_task = true;
      tasks::set_rgb_led::SetRgbLed::populateInput(action_config["params"],
                                                   *input);
      action.input = std::unique_ptr<tasks::BaseTask::Input>(input);

    } else {
      // Unknown action
      TRACEF("Unknown action type: %s\r\n",
             action_config["type"].as<const char*>());
      actions.pop_back();
      config.action_count--;
    }
    TRACELN(F("Parsed"));
  }
}

}  // namespace lac
}  // namespace inamata