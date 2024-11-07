#include "config.h"

#include "configuration.h"

#ifndef BEHAVIOR_BASED

namespace inamata {
namespace tasks {
namespace fixed {

bool startFixedTasks(const ServiceGetters& services, Scheduler& scheduler,
                     const JsonObjectConst& behavior_config) {
  return false;
}

}  // namespace fixed
}  // namespace tasks
}  // namespace inamata

#endif