#pragma once

#include <TaskSchedulerDeclarations.h>

#include <memory>

#include "base_task.h"
#include "managers/service_getters.h"
#include "task_factory.h"

namespace inamata {
namespace tasks {

/**
 * This class is responsible for task creation, memory management and deletion.
 *
 * For usage instructions see the API.md documentation
 */
class TaskController {
  friend class TaskRemovalTask;

 public:
  TaskController(Scheduler& scheduler, TaskFactory& factory);
  virtual ~TaskController() = default;

  static const String& type();

  void setServices(ServiceGetters services);

  /**
   * Callback for messages regarding tasks from the server
   *
   * @param message The message as a JSON doc
   */
  void handleCallback(const JsonObjectConst& message);

  /**
   * Gets all currently running task IDs
   *
   * \return A vector with the task IDs
   */
  std::vector<utils::UUID> getTaskIDs();

  /**
   * Start a new task
   *
   * @param services System services such as WebSocket and Storage
   * @param parameters JSON object with the parameters to start a task
   * @return True on success
   */
  ErrorResult startTask(const ServiceGetters& services,
                        const JsonObjectConst& parameters);

 private:
  /**
   * Command a task to end
   *
   * Tasks are only fully removed by the TaskRemovalTask. It will send a stop
   * success result when it succeeds, so only report errors and success for
   * tasks that were not found (idempotent behavior).
   *
   * @param parameters JSON object with the parameters to stop a task
   * @param stop_results JSON array with stop results
   * @return True on success
   */
  void stopTask(JsonObjectConst parameters, JsonArray stop_results);

  /**
   * Find the base task by UUID
   *
   * \param uuid The ID of the base task
   * \return Pointer to the found base task
   */
  BaseTask* findTask(const utils::UUID& uuid);

  const String& getTaskType(Task* task);

  Scheduler& scheduler_;
  TaskFactory& factory_;
  ServiceGetters services_;

  static const __FlashStringHelper* task_command_key_;
  static const __FlashStringHelper* start_command_key_;
  static const __FlashStringHelper* stop_command_key_;

  static const __FlashStringHelper* task_results_key_;

  static const String task_type_system_task_;
  static const __FlashStringHelper* task_not_found_error_;
};
}  // namespace tasks
}  // namespace inamata