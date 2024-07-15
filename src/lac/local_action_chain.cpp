#include "local_action_chain.h"

#include <tinyexpr.h>

#include "lac/if_action.h"
#include "lac/lac_controller.h"
#include "lac/math_action.h"
#include "tasks/get_values_task/get_values_task.h"
#include "tasks/read_sensor/read_sensor.h"
#include "tasks/set_rgb_led/set_rgb_led.h"
#include "tasks/set_value/set_value.h"

namespace inamata {
namespace lac {

LocalActionChain::LocalActionChain(const ServiceGetters& services,
                                   const JsonVariantConst& config,
                                   Scheduler& scheduler)
    : BaseTask(scheduler, Input(config[id_key_], true)),
      services_(services),
      task_scheduler_(scheduler) {
  setIterations(-1);
  setInterval(1000);

  if (!getTaskID().isValid()) {
    setInvalid(id_key_error_);
    return;
  }
  JsonArrayConst routine = config["routine"];
  actions_.reserve(routine.size());

  for (const JsonObjectConst& action_config : routine) {
    TRACEKJSON("action: ", action_config);
    actions_.emplace_back();
    Action& action = actions_.back();

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

#ifdef ESP32
    } else if (action_type == tasks::set_rgb_led::SetRgbLed::type()) {
      // Set RGB LED task
      action.type = Action::Type::SetRgbLed;
      auto input = new tasks::set_rgb_led::SetRgbLed::Input();
      input->local_task = true;
      tasks::set_rgb_led::SetRgbLed::populateInput(action_config["params"],
                                                   *input);
      action.input = std::unique_ptr<tasks::BaseTask::Input>(input);
#endif

    } else if (action_type == MathAction::type()) {
      // Math action
      action.type = Action::Type::Math;
      auto config = new MathAction::Config();
      MathAction::populateConfig(action_config, *config);
      action.config = std::unique_ptr<MathAction::Config>(config);
      Action::parseOuts(action_config["out"], action);

    } else if (action_type == IfAction::type()) {
      // If action
      action.type = Action::Type::If;
      auto config = new IfAction::Config();
      IfAction::populateConfig(action_config, *config, actions_);
      action.config = std::unique_ptr<IfAction::Config>(config);

    } else {
      // Unknown action
      TRACEF("Unknown action type: %s\n",
             action_config["type"].as<const char*>());
      actions_.pop_back();
    }
    TRACELN(F("Parsed"));
  }
  if (actions_.size() == 0) {
    setInvalid("No actions");
  }
}

const String& LocalActionChain::getType() const { return type(); }

const String& LocalActionChain::type() {
  static const String name{"LAC"};
  return name;
}

bool LocalActionChain::TaskCallback() {
  TRACEF("Actions: %i\n", actions_.size());
  Action* action = getCurrentAction();
  if (!action) {
    TRACELN(F("Stop LAC nullptr"));
    return false;
  }
  TRACEF("Action type %d, # %d\n", action->type, current_action_num_);
  switch (action->type) {
    case Action::Type::AlertSensor:
      handleAlertSensor(*action);
      break;
    case Action::Type::ReadButton:
      handleReadButton(*action);
      break;
    case Action::Type::ReadSensor:
      handleReadSensor(*action);
      break;
    case Action::Type::SetValue:
      handleSetValue(*action);
      break;
#ifdef ESP32
    case Action::Type::SetRgbLed:
      handleSetRgbLed(*action);
      break;
#endif
    case Action::Type::Math:
      handleMath(*action);
      break;
    case Action::Type::If:
      handleIf(*action);
      break;
    default:
      break;
  }
  TRACELN(F("LAC::TC::end"));
  return true;
}

void LocalActionChain::OnTaskDisable() {
  JsonDocument doc_out;
  doc_out[WebSocket::type_key_] = WebSocket::result_type_;
  JsonObject lac_results =
      doc_out[LacController::lac_command_key_].to<JsonObject>();
  JsonArray stop_results =
      lac_results[BaseTask::stop_command_key_].to<JsonArray>();
  addResultEntry(stop_results);
  services_.getWebSocket()->sendResults(doc_out.as<JsonObject>());
}

void LocalActionChain::handleAlertSensor(Action& action) {
  // If nullptr, start task
  TRACELN(F("1"));
  if (!action.input->task_id.isValid()) {
    TRACELN(F("Starting AS"));
    action.input->task_id = utils::UUID();
    tasks::alert_sensor::AlertSensor::Input* input =
        static_cast<tasks::alert_sensor::AlertSensor::Input*>(
            action.input.get());
    auto alert_sensor = new tasks::alert_sensor::AlertSensor(
        services_, task_scheduler_, *input);
    if (!alert_sensor->isValid()) {
      Serial.println(alert_sensor->getError().toString());
      alert_sensor->abort();
      delete alert_sensor;
      TRACELN(F("Failed AS"));
      return;
    }
    alert_sensor->handle_output_ =
        std::bind(&LocalActionChain::handleAlertSensorOutput, this, _1);
    alert_sensor->on_task_disable_ =
        std::bind(&LocalActionChain::startNextAction, this);
    TRACELN(F("Created AS"));
    // TODO: handle error case
  }
}

bool LocalActionChain::handleAlertSensorOutput(
    tasks::alert_sensor::AlertSensor::TriggerType trigger_type) {
  TRACELN(F("ASO"));
  return true;
}

void LocalActionChain::handleSetValue(Action& action) {
  // If nullptr, start task
  if (!action.input->task_id.isValid()) {
    TRACELN(F("Starting SV"));
    action.input->task_id = utils::UUID();
    tasks::set_value::SetValue::Input* input =
        static_cast<tasks::set_value::SetValue::Input*>(action.input.get());
    auto set_value = new tasks::set_value::SetValue(task_scheduler_, *input);
    if (!set_value->isValid()) {
      Serial.println(set_value->getError().toString());
      set_value->abort();
      delete set_value;
      TRACELN(F("Failed SV"));
      return;
    }
    set_value->handle_output_ =
        std::bind(&LocalActionChain::handleSetValueOutput, this, _1, _2);
    set_value->on_task_disable_ =
        std::bind(&LocalActionChain::startNextAction, this);
    TRACELN(F("Created SV"));
  }
}

#ifdef ESP32
void LocalActionChain::handleSetRgbLed(Action& action) {
  // If nullptr, start task
  if (!action.input->task_id.isValid()) {
    TRACELN(F("Starting SetRGB"));
    action.input->task_id = utils::UUID();
    tasks::set_rgb_led::SetRgbLed::Input* input =
        static_cast<tasks::set_rgb_led::SetRgbLed::Input*>(action.input.get());
    auto set_rgb_led =
        new tasks::set_rgb_led::SetRgbLed(task_scheduler_, *input);
    if (!set_rgb_led->isValid()) {
      Serial.println(set_rgb_led->getError().toString());
      set_rgb_led->abort();
      delete set_rgb_led;
      TRACELN(F("Failed RGB"));
      return;
    }
    set_rgb_led->on_task_disable_ =
        std::bind(&LocalActionChain::startNextAction, this);
    TRACELN(F("Created SetRGB"));
  }
}
#endif

void LocalActionChain::handleReadButton(Action& action) {}

void LocalActionChain::handleReadSensor(Action& action) {
  // If nullptr, start task
  if (!action.input->task_id.isValid()) {
    TRACELN(F("Starting RS"));
    action.input->task_id = utils::UUID();
    tasks::read_sensor::ReadSensor::Input* input =
        static_cast<tasks::read_sensor::ReadSensor::Input*>(action.input.get());
    auto read_value =
        new tasks::read_sensor::ReadSensor(services_, task_scheduler_, *input);
    if (!read_value->isValid()) {
      Serial.println(read_value->getError().toString());
      read_value->abort();
      delete read_value;
      TRACELN(F("Failed RS"));
      return;
    }
    read_value->handle_output_ =
        std::bind(&LocalActionChain::handleGetValuesOutput, this, _1, _2);
    read_value->on_task_disable_ =
        std::bind(&LocalActionChain::startNextAction, this);
    TRACELN(F("Created RS"));
  }
}

void LocalActionChain::handleSetValueOutput(utils::ValueUnit& value,
                                            tasks::set_value::SetValue& task) {
  TRACEF("Results: %f, dpt: %s\n", value.value,
         value.data_point_type.toString().c_str());
  Action* action = getCurrentAction();
  TRACEF("Outs: %d\n", action->action_outs.size());
  for (const ActionOut& action_out : action->action_outs) {
    TRACEF("Type: %d\n", action_out.type);
    switch (action_out.type) {
      case ActionOut::Type::Telemetry: {
        task.sendTelemetry(value, &getTaskID());
        break;
      }
      case ActionOut::Type::Variable: {
        // Search for the value with the matching DPT and set the variable
        Variable& variable = getVariableOrCreate(action_out.name);
        variable.value = value.value;
        variable.data_point_type = value.data_point_type;
        TRACEF("Var set %s : %s : %f\n", variable.name.c_str(),
               variable.data_point_type.toString().c_str(), variable.value);
        break;
      }
      default:
        TRACEF("Unknown out: %i\n", action_out.type);
        break;
    }
  }
}

void LocalActionChain::handleGetValuesOutput(
    peripheral::capabilities::GetValues::Result& result,
    tasks::get_values_task::GetValuesTask& task) {
  TRACEF("Results: %d, error: %s\n", result.values.size(),
         result.error.detail_.c_str());
  Action* action = getCurrentAction();
  TRACEF("Outs: %d\n", action->action_outs.size());
  for (const ActionOut& action_out : action->action_outs) {
    TRACEF("Type: %d\n", action_out.type);
    switch (action_out.type) {
      case ActionOut::Type::Telemetry: {
        task.sendTelemetry(result);
        break;
      }
      case ActionOut::Type::Variable: {
        // Search for the value with the matching DPT and set the variable
        Variable& variable = getVariableOrCreate(action_out.name);
        for (const auto& value : result.values) {
          if (value.data_point_type == action_out.data_point_type) {
            variable.value = value.value;
            variable.data_point_type = value.data_point_type;
            TRACEF("Var set %s : %s : %f\n", variable.name.c_str(),
                   variable.data_point_type.toString().c_str(), variable.value);
            continue;
          }
        }
        break;
      }
      default:
        TRACEF("Unknown out: %i\n", action_out.type);
        break;
    }
  }
}

void LocalActionChain::handleMath(Action& action) {
  // Prepare TinyExpr variables from LAC variables
  std::vector<te_variable> vars(variables_.size());
  TRACEF("Vars: %d\n", variables_.size());
  for (uint8_t i = 0; i < variables_.size(); i++) {
    vars[i].name = variables_[i].name.c_str();
    vars[i].address = &variables_[i].value;
    TRACEF("Var %s: %f\n", vars[i].name,
           *(static_cast<const float*>(vars[i].address)));
  }
  MathAction::Config* math_config =
      static_cast<MathAction::Config*>(action.config.get());

  // Compile and evaluate expression
  int err;
  te_expr* expr =
      te_compile(math_config->query.c_str(), vars.data(), vars.size(), &err);
  if (!expr) {
    // Compile failed
    TRACEF("Parse error at %d : %s\n", err, math_config->query.c_str());
    endRoutine("If compile failed");
  }
  const float value = te_eval(expr);
  TRACEF("Math eval: %f\n", value);

  // Set outputs
  for (const ActionOut& action_out : action.action_outs) {
    switch (action_out.type) {
      case ActionOut::Type::Variable: {
        Variable& variable = getVariableOrCreate(action_out.name);
        variable.value = value;
        variable.data_point_type = action_out.data_point_type;
        break;
      }
      default:
        TRACEF("Unknown out: %i\n", action_out.type);
        break;
    }
  }

  startNextAction();
}

void LocalActionChain::handleIf(Action& action) {
  // Prepare TinyExpr variables from LAC variables
  std::vector<te_variable> vars(variables_.size());
  for (uint8_t i = 0; i < variables_.size(); i++) {
    vars[i].name = variables_[i].name.c_str();
    vars[i].address = &variables_[i].value;
  }
  IfAction::Config* if_config =
      static_cast<IfAction::Config*>(action.config.get());

  // Compile and evaluate expression
  int err;
  te_expr* expr =
      te_compile(if_config->query.c_str(), vars.data(), vars.size(), &err);
  if (!expr) {
  }
  const float value = te_eval(expr);
  TRACEF("If eval: %f\n", value);

  // Run then branch or skip (0 == false, 1 == true)
  if (value < 0.5) {
    TRACEF("Skipping %d\n", if_config->action_count);
    skipActions(if_config->action_count);
  } else {
    TRACELN(F("Run then actions"));
  }
  startNextAction();
}

void LocalActionChain::startNextAction() {
  TRACELN(F("SNA"));
  // End the LAC if the last action was executed
  if (atRoutineEnd()) {
    disable();
    return;
  }
  current_action_num_++;
  forceNextIteration();
}

void LocalActionChain::skipActions(uint8_t count) {
  current_action_num_ += count;
}

Action* LocalActionChain::getCurrentAction() {
  if (current_action_num_ < 0 || atRoutineEnd()) {
    return nullptr;
  }
  return &actions_[current_action_num_];
};

bool LocalActionChain::atRoutineEnd() {
  return current_action_num_ >= actions_.size();
}

void LocalActionChain::endRoutine(const char* error) {
  current_action_num_ = actions_.size();
  return;
}

void LocalActionChain::addResultEntry(utils::UUID lac_id,
                                      const LacErrorResult& error,
                                      const JsonArray& results) {
  JsonObject result = results.add<JsonObject>();

  // Save whether the task could be started or the reason for failing
  result[WebSocket::uuid_key_] = lac_id.toString();
  if (error.isError()) {
    result[WebSocket::result_status_key_] = WebSocket::result_fail_name_;
    result[WebSocket::result_detail_key_] = error.detail_;
  } else {
    result[WebSocket::result_status_key_] = WebSocket::result_success_name_;
  }

  result[WebSocket::result_state_key_] = error.state_;
}

void LocalActionChain::addResultEntry(const JsonArray& results) {
  addResultEntry(getTaskID(), getLacError(), results);
}

LacErrorResult LocalActionChain::getLacError() {
  if (isValid()) {
    return LacErrorResult(getState());
  } else {
    return LacErrorResult(getType(), error_message_, getState());
  }
}

bool LocalActionChain::isInstalled() { return false; }

String LocalActionChain::getState() {
  if (isInstalled()) {
    if (getError().isError()) {
      return "install_fail";
    }
    if (atRoutineEnd()) {
      return "install_end";
    }
    return "install_run";
  }
  if (atRoutineEnd()) {
    return "none";
  }
  return "running";
}

Variable& LocalActionChain::getVariableOrCreate(const String& name) {
  auto by_name = [name](Variable& v) { return v.name == name; };

  auto it = std::find_if(begin(variables_), end(variables_), by_name);
  if (it != std::end(variables_)) {
    TRACEF("Get %s\n", name.c_str());
    return *it;
  } else {
    TRACEF("Create %s\n", name.c_str());
    variables_.push_back({.name = name});
    return variables_.back();
  }
}

const __FlashStringHelper* LocalActionChain::id_key_ = FPSTR("uuid");
const __FlashStringHelper* LocalActionChain::id_key_error_ =
    FPSTR("Missing property: uuid (uuid)");

}  // namespace lac
}  // namespace inamata