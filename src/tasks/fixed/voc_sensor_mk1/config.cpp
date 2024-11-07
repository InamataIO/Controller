#ifdef DEVICE_TYPE_VOC_SENSOR_MK1

#include "tasks/fixed/config.h"

#include "alert.h"
#include "calibrate_voc.h"
#include "configuration.h"
#include "poll_voc.h"

namespace inamata {
namespace tasks {
namespace fixed {

bool startFixedTasks(const ServiceGetters& services, Scheduler& scheduler,
                     const JsonObjectConst& behavior_config) {
  Alert* alert_task = new Alert(services, scheduler, behavior_config);
  ErrorResult error = alert_task->getError();
  if (error.isError()) {
    alert_task->abort();
    delete alert_task;
    return false;
  }

  CalibrateVoc* calibrate_voc_task = new CalibrateVoc(scheduler);
  error = calibrate_voc_task->getError();
  if (error.isError()) {
    calibrate_voc_task->abort();
    delete calibrate_voc_task;
    return false;
  }

  PollVoc* poll_voc_task = new PollVoc(services, scheduler, behavior_config);
  error = poll_voc_task->getError();
  if (error.isError()) {
    poll_voc_task->abort();
    delete poll_voc_task;
    return false;
  }

  return true;
}

}  // namespace fixed
}  // namespace tasks
}  // namespace inamata

#endif