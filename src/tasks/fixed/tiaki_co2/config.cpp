#ifdef DEVICE_TYPE_TIAKI_CO2_MONITOR

#include "tasks/fixed/config.h"

#include "alarms.h"
#include "anti_condensation.h"
#include "configuration.h"
#include "network_led.h"
#include "poll_remote_sensor.h"
#include "user_input.h"
#include "welcome.h"

namespace inamata {
namespace tasks {
namespace fixed {

bool startFixedTasks(const ServiceGetters& services, Scheduler& scheduler,
                     const JsonObjectConst& behavior_config) {
  Welcome* welcome_task = new Welcome(scheduler, behavior_config);
  ErrorResult error = welcome_task->getError();
  if (error.isError()) {
    TRACEF("Welcome fail: %s\n", error.toString().c_str());
    welcome_task->abort();
    delete welcome_task;
    return false;
  }
  UserInput* touch_input_task =
      new UserInput(services, scheduler, behavior_config);
  error = touch_input_task->getError();
  if (error.isError()) {
    TRACEF("Touch input fail: %s\n", error.toString().c_str());
    touch_input_task->abort();
    delete touch_input_task;
    return false;
  }
  Alarms* alarms_task = new Alarms(services, scheduler, behavior_config);
  error = alarms_task->getError();
  if (error.isError()) {
    TRACEF("Alarms fail: %s\n", error.toString().c_str());
    alarms_task->abort();
    delete alarms_task;
    return false;
  }
  PollRemoteSensor* poll_remote_sensor_task =
      new PollRemoteSensor(services, scheduler, behavior_config);
  error = poll_remote_sensor_task->getError();
  if (error.isError()) {
    TRACEF("Poll remote sensor fail: %s\n", error.toString().c_str());
    poll_remote_sensor_task->abort();
    delete poll_remote_sensor_task;
    return false;
  }
  AntiCondensation* anti_condensation_task =
      new AntiCondensation(scheduler, behavior_config);
  error = anti_condensation_task->getError();
  if (error.isError()) {
    TRACEF("AntiCondensation fail: %s\n", error.toString().c_str());
    anti_condensation_task->abort();
    delete anti_condensation_task;
    return false;
  }
  NetworkLed* network_led_task =
      new NetworkLed(services, scheduler, behavior_config);
  error = network_led_task->getError();
  if (error.isError()) {
    TRACEF("NetworkLed fail: %s\n", error.toString().c_str());
    network_led_task->abort();
    delete network_led_task;
    return false;
  }

  return true;
}

}  // namespace fixed
}  // namespace tasks
}  // namespace inamata

#endif