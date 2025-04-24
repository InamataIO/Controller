#include "logman_task.h"

namespace inamata {
namespace tasks {
namespace logging_man {

LoggingManagerTask::LoggingManagerTask(const ServiceGetters& services,
                                       Scheduler& scheduler)
    : BaseTask(scheduler, Input(nullptr, true)),
      scheduler_(scheduler),
      services_(services),
      log_manager_(services_.getLoggingManager()) {
  if (!isValid()) {
    return;
  }

  setIterations(TASK_FOREVER);
  Task::setInterval(std::chrono::milliseconds(1000).count());

  enable();
}

LoggingManagerTask::~LoggingManagerTask() {}

const String& LoggingManagerTask::getType() const { return type(); }

const String& LoggingManagerTask::type() {
  static const String name{"LoggingManager_Task"};
  return name;
}

bool LoggingManagerTask::OnTaskEnable() { return true; }

bool LoggingManagerTask::TaskCallback() { return true; }

}  // namespace logging_man
}  // namespace tasks
}  // namespace inamata