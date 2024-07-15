#include "base_task.h"

namespace inamata {
namespace tasks {

BaseTask::BaseTask(Scheduler& scheduler, const Input& input)
    : Task(&scheduler),
      local_task_(input.local_task),
      scheduler_(scheduler),
      task_id_(input.task_id) {}

void BaseTask::populateInput(const JsonObjectConst& parameters, Input& input) {
  JsonVariantConst uuid = parameters[task_id_key_];
  if (!uuid.isNull()) {
    input.task_id = uuid;
  }
}

bool BaseTask::OnEnable() {
  // Check that the task is valid before OnEnable is called
  if (isValid()) {
    // Run the actual setup code
    bool is_ok = OnTaskEnable();

    // Check if still valid. Stop and don't call callback or OnDisable
    if (isValid() && is_ok) {
      return true;
    }
  }
  return false;
}

bool BaseTask::OnTaskEnable() { return true; }

bool BaseTask::Callback() {
  // Check that the task is valid before it is executed
  if (isValid()) {
    // Run the actual task logic
    bool is_ok = TaskCallback();

    // Check if the task is still valid. Disable task if not
    if (isValid() && is_ok) {
      return true;
    }
  }
  disable();
  return true;
}

void BaseTask::OnDisable() {
  if (on_task_disable_) {
    on_task_disable_();
  }
  OnTaskDisable();
  if (skip_task_removal_) {
    return;
  }
  if (task_removal_callback_) {
    task_removal_callback_(*this);
  } else {
    TRACELN(F("Task removal callback not set. Rebooting in 10s"));
    delay(10000);
    ESP.restart();
  }
}

void BaseTask::OnTaskDisable() {}

void BaseTask::disableWithoutRemoval() {
  skip_task_removal_ = true;
  disable();
}

bool BaseTask::isValid() const { return is_valid_; }

ErrorResult BaseTask::getError() const {
  if (is_valid_) {
    return ErrorResult();
  } else {
    return ErrorResult(getType(), error_message_);
  }
}

const utils::UUID& BaseTask::getTaskID() const { return task_id_; }

bool BaseTask::isSystemTask() const { return !task_id_.isValid(); }

void BaseTask::setTaskRemovalCallback(std::function<void(Task&)> callback) {
  task_removal_callback_ = callback;
}

BaseTask* BaseTask::findTask(Scheduler& scheduler, utils::UUID task_id) {
  // Go through all tasks in the scheduler
  for (Task* task = scheduler.iFirst; task; task = task->iNext) {
    // Check if it is a base task
    BaseTask* base_task = dynamic_cast<BaseTask*>(task);
    // If the UUIDs match, return the task and end the search
    if (base_task && base_task->getTaskID() == task_id) {
      return base_task;
    }
  }

  return nullptr;
}

void BaseTask::setInvalid() { is_valid_ = false; }

void BaseTask::setInvalid(const String& error_message) {
  is_valid_ = false;
  error_message_ = error_message;
}

const __FlashStringHelper* BaseTask::peripheral_key_ = FPSTR("peripheral");
const __FlashStringHelper* BaseTask::peripheral_key_error_ =
    FPSTR("Missing property: peripheral (uuid)");
const __FlashStringHelper* BaseTask::task_id_key_ = FPSTR("uuid");
const __FlashStringHelper* BaseTask::task_id_key_error_ =
    FPSTR("Missing property: uuid (uuid)");

const __FlashStringHelper* BaseTask::start_command_key_ = FPSTR("start");
const __FlashStringHelper* BaseTask::stop_command_key_ = FPSTR("stop");

std::function<void(Task&)> BaseTask::task_removal_callback_ = nullptr;

}  // namespace tasks
}  // namespace inamata
