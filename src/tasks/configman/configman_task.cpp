#include "configman_task.h"

namespace inamata {
namespace tasks {
namespace config_man {

ConfigurationManagementTask::ConfigurationManagementTask(
    const ServiceGetters& services, Scheduler& scheduler)
    : BaseTask(scheduler, Input(nullptr, true)),
      scheduler_(scheduler),
      config_manager_(services.getConfigManager()) {
  if (!isValid()) {
    return;
  }

  const String error = config_manager_->init(services.getStorage());
  if (!error.isEmpty()) {
    setInvalid(error);
    return;
  }

  setIterations(TASK_FOREVER);
  enable();
}

ConfigurationManagementTask::~ConfigurationManagementTask() {}

const String& ConfigurationManagementTask::getType() const { return type(); }

const String& ConfigurationManagementTask::type() {
  static const String name{"ConfigurationManagement_Task"};
  return name;
}

bool ConfigurationManagementTask::TaskCallback() {
  Task::delay(std::chrono::milliseconds(interval_).count());
  config_manager_->loop();
  return true;
}

const std::chrono::milliseconds ConfigurationManagementTask::interval_(100);

}  // namespace config_man
}  // namespace tasks
}  // namespace inamata