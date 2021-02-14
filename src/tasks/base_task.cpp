#include "base_task.h"

namespace bernd_box {
namespace tasks {

BaseTask::BaseTask(Scheduler& scheduler, utils::UUID task_id)
    : Task(&scheduler), scheduler_(scheduler), task_id_(task_id) {}

BaseTask::BaseTask(Scheduler& scheduler, const JsonObjectConst& parameters)
    : Task(&scheduler), scheduler_(scheduler) {
  // Get and set the UUID to identify the task with the server
  task_id_ = utils::UUID(parameters[task_id_key_]);
  if (!task_id_.isValid()) {
    setInvalid(task_id_key_error_);
    return;
  }
}

bool BaseTask::OnEnable() {
  if (isValid()) {
    bool is_ok = OnTaskEnable();
    if (isValid() && is_ok) {
      return true;
    } else {
      return false;
    }
  } else {
    return false;
  }
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
    } else {
      return false;
    }
  } else {
    return false;
  }
}

void BaseTask::OnDisable() {
  OnTaskDisable();
  if (task_removal_callback_) {
    task_removal_callback_(*this);
  } else {
    Serial.println(F("Task removal callback not set. Rebooting in 10s"));
    delay(10000);
    ESP.restart();
  }
}

void BaseTask::OnTaskDisable() {}

bool BaseTask::isValid() const { return is_valid_; }

ErrorResult BaseTask::getError() const {
  if (is_valid_) {
    return ErrorResult();
  } else {
    return ErrorResult(getType(), error_message_);
  }
}

const utils::UUID& BaseTask::getTaskID() const { return task_id_; }

void BaseTask::setTaskRemovalCallback(std::function<void(Task&)> callback) {
  task_removal_callback_ = callback;
}

void BaseTask::setInvalid() { is_valid_ = false; }

void BaseTask::setInvalid(const String& error_message) {
  is_valid_ = false;
  error_message_ = error_message;
}

String BaseTask::peripheralNotFoundError(const utils::UUID& uuid) {
  String error(peripheral_not_found_error_);
  error += uuid.toString();
  return error;
}

const __FlashStringHelper* BaseTask::peripheral_key_ = F("peripheral");
const __FlashStringHelper* BaseTask::peripheral_key_error_ =
    F("Missing property: peripheral (uuid)");
const __FlashStringHelper* BaseTask::peripheral_not_found_error_ =
    F("Could not find peripheral: ");
const __FlashStringHelper* BaseTask::task_id_key_ = F("uuid");
const __FlashStringHelper* BaseTask::task_id_key_error_ =
    F("Missing property: uuid (uuid)");

std::function<void(Task&)> BaseTask::task_removal_callback_ = nullptr;

}  // namespace tasks
}  // namespace bernd_box
