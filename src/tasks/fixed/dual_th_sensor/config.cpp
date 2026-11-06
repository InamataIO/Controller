#ifdef DEVICE_TYPE_DUAL_TH_SENSOR

#include "tasks/fixed/config.h"

#include "alert.h"
#include "network_state.h"
#include "poll_sensors.h"

namespace inamata {
namespace tasks {
namespace fixed {

bool startFixedTasks(const ServiceGetters& services, Scheduler& scheduler,
                     const JsonObjectConst& behavior_config) {
  Alert* alert = new Alert(services, scheduler, behavior_config);
  ErrorResult error = alert->getError();
  if (error.isError()) {
    alert->abort();
    delete alert;
    return false;
  }

  PollSensors* poll_sensors = new PollSensors(services, scheduler);
  error = poll_sensors->getError();
  if (error.isError()) {
    poll_sensors->abort();
    delete poll_sensors;
    return false;
  }
  NetworkState* network_state =
      new NetworkState(services, scheduler, behavior_config);
  error = network_state->getError();
  if (error.isError()) {
    network_state->abort();
    delete network_state;
    return false;
  }
  return true;
}

}  // namespace fixed
}  // namespace tasks
}  // namespace inamata

#endif
