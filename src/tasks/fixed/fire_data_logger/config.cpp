#ifdef DEVICE_TYPE_FIRE_DATA_LOGGER

#include "tasks/fixed/config.h"

#include "alarms.h"
#include "configuration.h"
#include "heartbeat.h"
#include "network_state.h"
#include "telemetry.h"

namespace inamata {
namespace tasks {
namespace fixed {

bool startFixedTasks(const ServiceGetters& services, Scheduler& scheduler,
                     const JsonObjectConst& behavior_config) {
  NetworkState* cycle_colors_task =
      new NetworkState(services, scheduler, behavior_config);
  ErrorResult error = cycle_colors_task->getError();
  if (error.isError()) {
    Serial.println(error.toString());
    cycle_colors_task->abort();
    delete cycle_colors_task;
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

  Heartbeat* heartbeat_task = new Heartbeat(scheduler);
  error = heartbeat_task->getError();
  if (error.isError()) {
    Serial.println(error.toString());
    heartbeat_task->abort();
    delete heartbeat_task;
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

  return true;
}

}  // namespace fixed
}  // namespace tasks
}  // namespace inamata

#endif