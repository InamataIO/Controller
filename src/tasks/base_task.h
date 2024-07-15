#pragma once

#include <Arduino.h>
#include <TaskSchedulerDeclarations.h>

#include <functional>

#include "managers/logging.h"
#include "managers/types.h"
#include "utils/uuid.h"

namespace inamata {
namespace tasks {

class BaseTask : public Task {
  /**
   * All tasks should derive from this task that connects to the scheduler
   *
   * The constructor of derived tasks should
   *  - check input data and call setInvalid on errors
   *  - enable themselves after checks (enable() / enableDelayed())
   *
   * Local tasks are not registered on the server and will not send result
   * messages on task ends or initialization failures. Local action chains
   * should send this via a LAC result message while system tasks are ignored.
   */
 public:
  struct Input {
    Input(utils::UUID atask_id = nullptr, bool alocal_task = false)
        : task_id(atask_id), local_task(alocal_task) {}
    virtual ~Input() = default;
    utils::UUID task_id;
    bool local_task;
  };

  /**
   * Common base task sets the input variables.
   *
   * \param scheduler The scheduler that executes the tasks
   * \param input Variables including task ID and local task
   */
  BaseTask(Scheduler& scheduler, const Input& input);

  virtual ~BaseTask() = default;

  virtual const String& getType() const = 0;

  /**
   * @param[in] parameters The JSON object with the parameters
   * @param[out] input The parsed parameters used by the constructor
   */
  static void populateInput(const JsonObjectConst& parameters, Input& input);

  /**
   * Called by the task scheduler when the task is enabled and checks if the
   * task is valid
   *
   * \see OnTaskEnable()
   *
   * \return True if the task is valid
   */
  bool OnEnable() final;

  /**
   * For derived classes to perform init tasks and called when the task is
   * enabled
   *
   * \see OnEnable()
   *
   * \return True if the derived task inited successfully
   */
  virtual bool OnTaskEnable();

  /**
   * Called by the task scheduler when the task is to be executed
   *
   * \see TaskCallback()
   *
   * \return True if the task performed work
   */
  bool Callback() final;

  /**
   * For derived classes to perform execution logic.
   *
   * Use the setInvalid() functions to set error states. The task is cleaned up
   * and the error reported if it is not valid before or after being executed.
   *
   * \see Callback()
   *
   * \return True to continue, false to end normally
   */
  virtual bool TaskCallback() = 0;

  /**
   * Called by the task scheduler and triggers task removal
   *
   * Contains a static TaskRemovalTask which is tasked with removing disabled
   * tasks.
   *
   * \see OnTaskDisable()
   */
  void OnDisable() final;

  /**
   * For derived classes to perform clean up
   *
   * \see OnDisable()
   */
  virtual void OnTaskDisable();

  /**
   * Calls base disable() but skips handling by task removal task
   *
   * Use this instead of disable() when you will delete the task yourself.
   * This avoids sending a stop result and calling delete on the task.
   */
  void disableWithoutRemoval();

  /**
   * Check if the task is in a valid state
   *
   * \return True if it is in a valid state
   */
  bool isValid() const;

  /**
   * Gets the task's error state, which gives the who and detail information
   *
   * \return The task's error state
   */
  ErrorResult getError() const;

  /**
   * Gets the task's UUID, which was created locally or on the server
   *
   * \return The task's UUID
   */
  const utils::UUID& getTaskID() const;

  /**
   * Checks if it is a system task
   *
   * \return True if it is a system task
   */
  bool isSystemTask() const;

  /**
   * Sets the callback which accepts tasks to be removed
   *
   * \param callback The function to call to add a task to the removal queue
   */
  static void setTaskRemovalCallback(std::function<void(Task&)> callback);

  static BaseTask* findTask(Scheduler& scheduler, utils::UUID task_id);

  // Whether the task was started locally or by the server
  bool local_task_ = false;
  /// Function called on task disable (task end)
  std::function<void()> on_task_disable_;

  static const __FlashStringHelper* peripheral_key_;
  static const __FlashStringHelper* peripheral_key_error_;
  static const __FlashStringHelper* task_id_key_;
  static const __FlashStringHelper* task_id_key_error_;

  static const __FlashStringHelper* start_command_key_;
  static const __FlashStringHelper* stop_command_key_;

 protected:
  /**
   * Mark the task as being in an invalid state
   *
   * \see isValid()
   */
  void setInvalid();

  /**
   * Mark the task as being invalid and the cause
   *
   * \see isValid(), getError()
   *
   * \param error_message The cause for being invalid
   */
  void setInvalid(const String& error_message);

  /// Whether the task is in a valid or invalid state
  bool is_valid_ = true;
  /// The cause for being in an invalid state
  String error_message_;

 private:
  /// The scheduler the task is bound to
  Scheduler& scheduler_;
  /// The task's identifier
  utils::UUID task_id_ = utils::UUID(nullptr);
  /// Add task to removal queue callback
  static std::function<void(Task&)> task_removal_callback_;
  /// Skip deletion by task removal task
  bool skip_task_removal_ = false;
};

}  // namespace tasks
}  // namespace inamata
