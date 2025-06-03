#include "configman_task.h"

#include "managers/time_manager.h"

namespace inamata {
namespace tasks {
namespace config_man {

using namespace std::placeholders;

ConfigurationManagementTask::ConfigurationManagementTask(
    const ServiceGetters& services, Scheduler& scheduler)
    : BaseTask(scheduler, Input(nullptr, true)),
      scheduler_(scheduler),
      config_manager_(services.getConfigManager()) {
  if (!isValid()) {
    return;
  }

  if (config_manager_ == nullptr) {
    setInvalid(ServiceGetters::config_manager_nullptr_error_);
    return;
  }
  const String error = config_manager_->init(services.getStorage());
  if (!error.isEmpty()) {
    setInvalid(error);
    return;
  }

  std::shared_ptr<BleServer> ble_server = services.getBleServer();
  if (ble_server == nullptr) {
    setInvalid(ServiceGetters::ble_server_nullptr_error_);
    return;
  }
  ble_server->user_data_handlers_.push_back(std::bind(
      &ConfigManager::handleImprovUserData, config_manager_.get(), _1));
#ifdef RTC_MANAGER
  ble_server->user_data_handlers_.push_back(&TimeManager::handleImprovUserData);
#endif

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