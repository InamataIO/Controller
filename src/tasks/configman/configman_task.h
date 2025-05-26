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
  bool TaskCallback() final;

  Scheduler& scheduler_;

  std::shared_ptr<ConfigManager> config_manager_;
  static const std::chrono::milliseconds interval_;
};

}  // namespace config_man
}  // namespace tasks
}  // namespace inamata
