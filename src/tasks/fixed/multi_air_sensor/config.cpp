#ifdef DEVICE_TYPE_MULTI_AIR_SENSOR

#include "tasks/fixed/config.h"

#include "aging.h"
#include "alarms.h"
#include "auto_calibrate.h"
#include "configuration.h"
#include "o2_calibration.h"
#include "telemetry.h"
#include "update_display.h"

namespace inamata {
namespace tasks {
namespace fixed {

bool startFixedTasks(const ServiceGetters& services, Scheduler& scheduler,
                     const JsonObjectConst& behavior_config) {
  UpdateDisplay* network_state_task =
      new UpdateDisplay(services, scheduler, behavior_config);
  ErrorResult error = network_state_task->getError();
  if (error.isError()) {
    Serial.println(error.toString());
    network_state_task->abort();
    delete network_state_task;
    return false;
  }

  Telemetry* telemetry_task = new Telemetry(services, scheduler);
  error = telemetry_task->getError();
  if (error.isError()) {
    Serial.println(error.toString());
    telemetry_task->abort();
    delete telemetry_task;
    return false;
  }

  AutoCalibrate* auto_calibrate_task =
      new AutoCalibrate(services, scheduler, behavior_config);
  error = auto_calibrate_task->getError();
  if (error.isError()) {
    Serial.println(error.toString());
    auto_calibrate_task->abort();
    delete auto_calibrate_task;
    return false;
  }

  Alarms* alarms_task = new Alarms(services, scheduler, behavior_config);
  error = alarms_task->getError();
  if (error.isError()) {
    Serial.println(error.toString());
    alarms_task->abort();
    delete alarms_task;
    return false;
  }

  Aging* aging_task = new Aging(services, scheduler, behavior_config);
  error = aging_task->getError();
  if (error.isError()) {
    Serial.println(error.toString());
    aging_task->abort();
    delete aging_task;
    return false;
  }

  O2Calibration* o2_calibration_task =
      new O2Calibration(services, scheduler, behavior_config);
  error = o2_calibration_task->getError();
  if (error.isError()) {
    Serial.println(error.toString());
    o2_calibration_task->abort();
    delete o2_calibration_task;
    return false;
  }

  return true;
}

}  // namespace fixed
}  // namespace tasks
}  // namespace inamata

#endif