#ifdef DEVICE_TYPE_FIRE_DATA_LOGGER

#include "tasks/fixed/config.h"

#include "alarms.h"
#include "configuration.h"
#include "heartbeat.h"
#include "log_inputs.h"
#include "network_state.h"
#include "telemetry.h"

namespace inamata {
namespace tasks {
namespace fixed {

bool startFixedTasks(const ServiceGetters& services, Scheduler& scheduler,
                     const JsonObjectConst& behavior_config) {
  NetworkState* network_state_task =
      new NetworkState(services, scheduler, behavior_config);
  ErrorResult error = network_state_task->getError();
  if (error.isError()) {
    Serial.println(error.toString());
    network_state_task->abort();
    delete network_state_task;
    return false;
  }

  Alarms* alarms_task = new Alarms(services, scheduler, behavior_config);
  if (!alarms_task->isValid()) {
    Serial.println(alarms_task->getError().toString());
    alarms_task->abort();
    delete alarms_task;
    return false;
  }

  Heartbeat* heartbeat_task = new Heartbeat(scheduler);
  if (!heartbeat_task->isValid()) {
    Serial.println(heartbeat_task->getError().toString());
    heartbeat_task->abort();
    delete heartbeat_task;
    return false;
  }

  Telemetry* telemetry_task = new Telemetry(services, scheduler);
  if (!telemetry_task->isValid()) {
    Serial.println(telemetry_task->getError().toString());
    telemetry_task->abort();
    delete telemetry_task;
    return false;
  }

  LogInputs* log_inputs_task = new LogInputs(services, scheduler);
  if (!log_inputs_task->isValid()) {
    Serial.println(log_inputs_task->getError().toString());
    log_inputs_task->abort();
    delete log_inputs_task;
    return false;
  }

  return true;
}

}  // namespace fixed
}  // namespace tasks
}  // namespace inamata

#endif