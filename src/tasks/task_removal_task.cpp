#include "task_removal_task.h"

#include <ArduinoJson.h>
#include <TaskSchedulerDeclarations.h>

#include <functional>

#include "tasks/task_controller.h"

namespace inamata {
namespace tasks {

using namespace std::placeholders;

TaskRemovalTask::TaskRemovalTask(Scheduler& scheduler) : Task(&scheduler) {
  BaseTask::setTaskRemovalCallback(std::bind(&TaskRemovalTask::add, this, _1));
}

const String& TaskRemovalTask::type() {
  static const String name{"TaskRemovalTask"};
  return name;
}

void TaskRemovalTask::setServices(ServiceGetters services) {
  services_ = services;
}

void TaskRemovalTask::add(Task& pt) {
  tasks_.insert(&pt);
  setIterations(1);
  enableIfNot();
}

bool TaskRemovalTask::Callback() {
  if (tasks_.empty()) {
    return true;
  }

  JsonDocument doc_out;
  doc_out[WebSocket::type_key_] = WebSocket::result_type_;
  JsonObject task_results =
      doc_out[TaskController::task_results_key_].to<JsonObject>();
  JsonArray stop_results =
      task_results[BaseTask::stop_command_key_].to<JsonArray>();

  for (auto it = tasks_.begin(); it != tasks_.end();) {
    Task* task = *it;
    BaseTask* base_task = dynamic_cast<BaseTask*>(task);
    TRACEF("Deleting: %s (l: %i, s: %i, id: %s)\n", base_task->getType(),
           base_task->local_task_, base_task->isSystemTask(),
           base_task->getTaskID().toString().c_str());

    // - Local, system tasks have static memory and should not be deleted
    // - Server started tasks should be deleted and sent to the server
    // - Local, non-system tasks should be deleted but not sent to the server
    if (base_task && !base_task->isSystemTask()) {
      if (!base_task->local_task_) {
        String task_id = base_task->getTaskID().toString();
        WebSocket::addResultEntry(task_id, base_task->getError(), stop_results);
      }
      delete base_task;
    }
    it = tasks_.erase(it);
  }
  tasks_.clear();
  if (stop_results.size()) {
    std::shared_ptr<WebSocket> web_socket = services_.getWebSocket();
    if (web_socket != nullptr) {
      web_socket->sendResults(doc_out.as<JsonObject>());
    } else {
      TRACELN(ErrorResult(type(), ServiceGetters::web_socket_nullptr_error_)
                  .toString());
    }
  }

  return true;
}

}  // namespace tasks
}  // namespace inamata