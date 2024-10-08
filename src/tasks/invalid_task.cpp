#include "invalid_task.h"

namespace inamata {
namespace tasks {

InvalidTask::InvalidTask(Scheduler& scheduler, const String& error)
    : BaseTask(scheduler, Input(nullptr, true)) {
  setInvalid(error);
}

bool InvalidTask::TaskCallback() { return false; }

const String& InvalidTask::getType() const { return type(); }

const String& InvalidTask::type() {
  static const String name{"InvalidTask"};
  return name;
}

}  // namespace tasks
}  // namespace inamata