#include "configman_task.h"

namespace inamata {
namespace tasks {
namespace config_man {

ConfigurationManagementTask::ConfigurationManagementTask(
    const ServiceGetters& services, Scheduler& scheduler)
    : BaseTask(scheduler, Input(nullptr, true)),
      scheduler_(scheduler),
      services_(services),
      config_manager_(services_.getConfigManager()) {
  if (!isValid()) {
    return;
  }

  setIterations(TASK_FOREVER);
  Task::setInterval(
      std::chrono::milliseconds(SERIAL_DATA_POLL_RATE_MS).count());

  enable();
}

ConfigurationManagementTask::~ConfigurationManagementTask() {}

const String& ConfigurationManagementTask::getType() const { return type(); }

const String& ConfigurationManagementTask::type() {
  static const String name{"ConfigurationManagement_Task"};
  return name;
}

bool ConfigurationManagementTask::OnTaskEnable() {
  config_manager_->initConfigMenuManager();
  return true;
}

bool ConfigurationManagementTask::TaskCallback() {
  config_manager_->loop();
  return true;
}

}  // namespace config_man
}  // namespace tasks
}  // namespace inamata