#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>

#include <vector>

#include "lac/action.h"
#include "managers/service_getters.h"
#include "tasks/alert_sensor/alert_sensor.h"
#include "tasks/base_task.h"
#include "tasks/set_value/set_value.h"
#include "utils/uuid.h"

namespace inamata {
namespace lac {

struct LacErrorResult : public ErrorResult {
  LacErrorResult(String who, String detail, String state = "none")
      : ErrorResult(who, detail), state_(state) {}
  LacErrorResult(String state) : ErrorResult(), state_(state) {}
  virtual ~LacErrorResult() = default;

  /// The LAC's state
  String state_;
};

class LocalActionChain : public tasks::BaseTask {
 public:
  LocalActionChain(const ServiceGetters& services,
                   const JsonVariantConst& config, Scheduler& scheduler);
  virtual ~LocalActionChain() = default;

  const String& getType() const final;
  static const String& type();

  bool TaskCallback();
  void OnTaskDisable();

  static void addResultEntry(utils::UUID lac_id, const LacErrorResult& error,
                             const JsonArray& results);

  LacErrorResult getLacError();

  bool isInstalled();

  String getState();

  static const __FlashStringHelper* id_key_;
  static const __FlashStringHelper* id_key_error_;

 private:
  void handleAlertSensor(Action& action);
  bool handleAlertSensorOutput(
      tasks::alert_sensor::AlertSensor::TriggerType trigger_type);

  void handleReadButton(Action& action);
  void handleReadSensor(Action& action);
  void handleSetValue(Action& action);
#ifdef ESP32
  void handleSetRgbLed(Action& action);
#endif

  void handleSetValueOutput(utils::ValueUnit& value,
                            tasks::set_value::SetValue& task);
  void handleGetValuesOutput(
      peripheral::capabilities::GetValues::Result& result,
      tasks::get_values_task::GetValuesTask& task);

  void handleMath(Action& action);
  void handleIf(Action& action);

  /**
   * Start the next action in the chain
   */
  void startNextAction();
  /**
   * Skip the specified number of actions (used by if action on false query)
   *
   * \param count The number of actions to skip
   */
  void skipActions(uint8_t count);

  /**
   * Get the currently active action if currently running
   *
   * \return The active action or nullptr if not started / ended
   */
  Action* getCurrentAction();

  /**
   * Check if the last action of the chain has been run
   */
  bool atRoutineEnd();
  /**
   * TODO: Abort routine due to error
   */
  void endRoutine(const char* error);

  void addResultEntry(const JsonArray& results);

  /**
   * Return a variable scoped to the LAC
   *
   * \param name Name of the variable to get or create
   * \return The variable of the specified name
   */
  Variable& getVariableOrCreate(const String& name);

  /// The ID of the LAC is saved in BaseTask::task_id_;
  // utils::UUID id_{nullptr};
  std::vector<Action> actions_;
  size_t current_action_num_ = 0;
  String error_;
  /// The variabls created by LAC actions
  std::vector<Variable> variables_;

  const ServiceGetters& services_;
  // Scheduler to pass use for started tasks
  Scheduler& task_scheduler_;
};

}  // namespace lac
}  // namespace inamata