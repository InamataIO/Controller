#pragma once

#include <TaskSchedulerDeclarations.h>

#include <chrono>

#include "managers/service_getters.h"
#include "tasks/base_task.h"

#define SERIAL_DATA_POLL_RATE_MS 100

namespace inamata {
namespace tasks {
namespace config_man {

class ConfigurationManagementTask : public BaseTask {
 public:
  ConfigurationManagementTask(const ServiceGetters& services,
                              Scheduler& scheduler);
  virtual ~ConfigurationManagementTask();

  const String& getType() const final;
  static const String& type();

 private:
  bool OnTaskEnable() final;
  bool TaskCallback() final;

  Scheduler& scheduler_;
  ServiceGetters services_;

  std::shared_ptr<ConfigManager> config_manager_;
};

}  // namespace config_man
}  // namespace tasks
}  // namespace inamata
