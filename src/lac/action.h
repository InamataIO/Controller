#pragma once

#include <Arduino.h>

#include <memory>
#include <vector>

#include "tasks/base_task.h"

namespace inamata {
namespace lac {

/**
 * Config used for action outputs (save to variables, send as telemetry)
 */
struct ActionOut {
  enum class Type { Telemetry, Variable };

  Type type;
  String name;
  utils::UUID data_point_type;

  static const __FlashStringHelper* telemetry_key_;
  static const __FlashStringHelper* variable_key_;
};

/**
 * Base config for a LAC action
 */
struct Action {
  enum class Type {
    AlertSensor,
    If,
    Math,
    PollSensor,
    ReadButton,
    ReadSensor,
    SetValue,
    SetRgbLed,
  };

  struct Config {
    virtual ~Config() = default;
  };

  static void parseOuts(JsonArrayConst targets, Action& action);

  Type type;
  std::unique_ptr<tasks::BaseTask::Input> input;
  std::unique_ptr<Config> config;
  // std::unique_ptr<ActionSource> source;
  std::vector<ActionOut> action_outs;

  static const __FlashStringHelper* query_key_;
};

/**
 * Variable saved and read by LAC actions
 */
struct Variable {
  String name;
  float value;
  utils::UUID data_point_type;
};

}  // namespace lac
}  // namespace inamata
