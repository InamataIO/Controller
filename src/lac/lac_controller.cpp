#include "lac/lac_controller.h"

#include "tasks/base_task.h"

namespace inamata {
namespace lac {

LacController::LacController(Scheduler& scheduler) : scheduler_(scheduler) {}

const String& LacController::type() {
  static const String name{"LacController"};
  return name;
}

void LacController::setServices(ServiceGetters services) {
  services_ = services;
}

void LacController::handleCallback(const JsonObjectConst& message) {
  // Check if any task commands have to be processed
  JsonVariantConst lac_commands = message[lac_command_key_];
  if (!lac_commands) {
    return;
  }
  TRACELN(F("Handling lac cmd"));

  // Init the result doc with type and the request ID
  JsonDocument doc_out;
  doc_out[WebSocket::type_key_] = WebSocket::result_type_;
  JsonVariantConst request_id = message[WebSocket::request_id_key_];
  if (request_id) {
    doc_out[WebSocket::request_id_key_] = request_id;
  }
  JsonObject lac_results = doc_out[lac_command_key_].to<JsonObject>();

  JsonObjectConst start_command =
      lac_commands[tasks::BaseTask::start_command_key_];
  if (start_command) {
    LacErrorResult error = startLac(services_, start_command);
    JsonArray start_results =
        lac_results[tasks::BaseTask::start_command_key_].to<JsonArray>();
    LocalActionChain::addResultEntry(start_command[LocalActionChain::id_key_],
                                     error, start_results);
  } else {
    Serial.println("No start cmds");
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

LacErrorResult LacController::startLac(const ServiceGetters& services,
                                       const JsonObjectConst& config) {
  // If the LAC already exists, stop it first
  utils::UUID lac_id = config[LocalActionChain::id_key_];
  if (!lac_id.isValid()) {
    return LacErrorResult(type(), LocalActionChain::id_key_error_);
  }

  tasks::BaseTask* base_task = tasks::BaseTask::findTask(scheduler_, lac_id);
  if (base_task != nullptr) {
    if (!base_task->local_task_) {
      return LacErrorResult(type(), F("Task not a LAC"));
    }
    base_task->disableWithoutRemoval();
    delete base_task;
  }

  LocalActionChain* lac = new LocalActionChain(services, config, scheduler_);
  lac->enable();
  LacErrorResult error = lac->getLacError();
  // On error, directly delete task without using the TaskRemovalTask to
  // avoid sending double stop notifications to the server.
  if (error.isError()) {
    // Call abort to ensure, OnDisable won't be called as otherwise it will
    // be added to the task removal task which will try to double free
    // memory, thereby causing a segmentation fault.
    lac->abort();
    delete lac;
  }
  return error;
}

const __FlashStringHelper* LacController::lac_command_key_ = FPSTR("lac");

}  // namespace lac
}  // namespace inamata