#include "task_controller.h"

#include <ArduinoJson.h>

namespace inamata {
namespace tasks {

TaskController::TaskController(Scheduler& scheduler, TaskFactory& factory)
    : scheduler_(scheduler), factory_(factory) {};

const String& TaskController::type() {
  static const String name{"TaskController"};
  return name;
}

void TaskController::setServices(ServiceGetters services) {
  services_ = services;
}

void TaskController::handleCallback(const JsonObjectConst& message) {
  // Check if any task commands have to be processed
  JsonVariantConst task_commands = message[task_command_key_];
  if (!task_commands) {
    return;
  }
  TRACELN(F("Handling task cmd"));

  // Init the result doc with type and the request ID
  JsonDocument doc_out;
  doc_out[WebSocket::type_key_] = WebSocket::result_type_;
  JsonVariantConst request_id = message[WebSocket::request_id_key_];
  if (request_id) {
    doc_out[WebSocket::request_id_key_] = request_id;
  }
  JsonObject task_results = doc_out[task_results_key_].to<JsonObject>();

  // Start a task for each command and store the result
  JsonArrayConst start_commands =
      task_commands[BaseTask::start_command_key_].as<JsonArrayConst>();
  if (start_commands) {
    JsonArray start_results =
        task_results[BaseTask::start_command_key_].to<JsonArray>();
    for (JsonVariantConst start_command : start_commands) {
      ErrorResult error = startTask(services_, start_command);
      WebSocket::addResultEntry(start_command[BaseTask::task_id_key_], error,
                                start_results);
    }
  }

  // Remove a task for each command and store the result
  JsonArrayConst stop_commands =
      task_commands[BaseTask::stop_command_key_].as<JsonArrayConst>();
  if (stop_commands) {
    JsonArray stop_results =
        task_results[BaseTask::stop_command_key_].to<JsonArray>();
    for (JsonVariantConst stop_command : stop_commands) {
      stopTask(stop_command, stop_results);
    }
  }

  // Send the command results
  std::shared_ptr<WebSocket> web_socket = services_.getWebSocket();
  if (web_socket != nullptr) {
    web_socket->sendResults(doc_out.as<JsonObject>());
  } else {
    TRACELN(
        ErrorResult(type(), services_.web_socket_nullptr_error_).toString());
  }
}

std::vector<utils::UUID> TaskController::getTaskIDs() {
  std::vector<utils::UUID> task_ids;

  for (Task* task = scheduler_.getFirstTask(); task != NULL;
       task = task->getNextTask()) {
    BaseTask* base_task = dynamic_cast<BaseTask*>(task);
    if (base_task) {
      task_ids.push_back(base_task->getTaskID());
    }
  }

  return task_ids;
}

ErrorResult TaskController::startTask(const ServiceGetters& services,
                                      const JsonObjectConst& parameters) {
  // If the task already exists, stop it first. Directly calls OnDisable
  utils::UUID task_id = parameters[BaseTask::task_id_key_];
  if (!task_id.isValid()) {
    return ErrorResult(type(), BaseTask::task_id_key_error_);
  }

  BaseTask* base_task = BaseTask::findTask(scheduler_, task_id);
  if (base_task != nullptr) {
    if (base_task->local_task_) {
      return ErrorResult(type(), F("Can't restart local tasks"));
    }
    TRACEF("Restarting task: %s\r\n", task_id.toString().c_str());
    base_task->disableWithoutRemoval();
    delete base_task;
  }

  // Create a task and check if a nullptr was returned
  BaseTask* task = factory_.startTask(services, parameters);
  if (task) {
    ErrorResult error = task->getError();
    // On error, directly delete task without using the TaskRemovalTask to
    // avoid sending double stop notifications to the server.
    if (error.isError()) {
      // Call abort to ensure, OnDisable won't be called as otherwise it will
      // be added to the task removal task which will try to double free
      // memory, thereby causing a segmentation fault.
      task->abort();
      delete task;
    }
    return error;
  } else {
    return ErrorResult(type(), "Unable to start task");
  }
}

void TaskController::stopTask(JsonObjectConst parameters,
                              JsonArray stop_results) {
  const char* task_uuid_str = parameters[BaseTask::task_id_key_];
  utils::UUID task_uuid(task_uuid_str);
  if (!task_uuid.isValid()) {
    auto error = ErrorResult(type(), BaseTask::task_id_key_error_);
    WebSocket::addResultEntry(task_uuid_str, error, stop_results);
    return;
  }

  BaseTask* base_task = BaseTask::findTask(scheduler_, task_uuid);
  if (base_task) {
    if (base_task->local_task_) {
      auto error = ErrorResult(type(), F("Can't stop local tasks"));
      WebSocket::addResultEntry(task_uuid_str, error, stop_results);
      return;
    }
    base_task->disable();
  } else {
    // If the task is not found, return success (idempotent behavior)
    WebSocket::addResultEntry(task_uuid_str, ErrorResult(), stop_results);
  }
}

const String& TaskController::getTaskType(Task* task) {
  BaseTask* base_task = dynamic_cast<BaseTask*>(task);
  if (base_task) {
    return base_task->getType();
  } else {
    return task_type_system_task_;
  }
}

const __FlashStringHelper* TaskController::task_command_key_ = FPSTR("task");
const __FlashStringHelper* TaskController::start_command_key_ = FPSTR("start");
const __FlashStringHelper* TaskController::stop_command_key_ = FPSTR("stop");

const __FlashStringHelper* TaskController::task_results_key_ = FPSTR("task");
const __FlashStringHelper* TaskController::task_not_found_error_ =
    FPSTR("Could not find task");

const String TaskController::task_type_system_task_{"SystemTask"};
}  // namespace tasks
}  // namespace inamata