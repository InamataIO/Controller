#pragma once

#include <Arduino.h>
#include <TaskSchedulerDeclarations.h>

#include "managers/service_getters.h"

namespace inamata {
namespace tasks {
namespace fixed {

bool startFixedTasks(const ServiceGetters& services, Scheduler& scheduler,
                     const JsonObjectConst& behavior_config);

}  // namespace fixed
}  // namespace tasks
}  // namespace inamata