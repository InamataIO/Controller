#pragma once

#include <TaskSchedulerDeclarations.h>

#include <chrono>

#include "managers/service_getters.h"
#include "tasks/base_task.h"

namespace inamata {
namespace tasks {
namespace logging_man {

class LoggingManagerTask : public BaseTask {
 public:
  LoggingManagerTask(const ServiceGetters& services, Scheduler& scheduler);
  virtual ~LoggingManagerTask();

  const String& getType() const final;
  static const String& type();

 private:
  bool OnTaskEnable() final;
  bool TaskCallback() final;

  Scheduler& scheduler_;
  ServiceGetters services_;

  std::shared_ptr<LoggingManager> log_manager_;
};

}  // namespace logging_man
}  // namespace tasks
}  // namespace inamata
