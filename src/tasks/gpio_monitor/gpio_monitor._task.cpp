#include "PCA9539.h"
#include "gpio_monitor_task.h"

namespace inamata {
namespace tasks {
namespace gpio_monitor {

#define IO_EXPANDER1_ADDRESS 0x74
#define IO_EXPANDER2_ADDRESS 0x75

#define IO_EXPANDER_RESET_CONTROL 45

PCA9539 ioExpanderA(IO_EXPANDER1_ADDRESS);
PCA9539 ioExpanderB(IO_EXPANDER2_ADDRESS);

GPIO_MonitorTask::GPIO_MonitorTask(const ServiceGetters& services,
                                   Scheduler& scheduler)
    : BaseTask(scheduler, Input(nullptr, true)),
      scheduler_(scheduler),
      services_(services) {
  if (!isValid()) {
    return;
  }

  setIterations(TASK_FOREVER);
  Task::setInterval(std::chrono::milliseconds(GPIO_MONITOR_RATE_MS).count());

  enable();
}

GPIO_MonitorTask::~GPIO_MonitorTask() {}
const String& GPIO_MonitorTask::getType() const { return type(); }
const String& GPIO_MonitorTask::type() {
  static const String name{"GPIO_Monitor_Task"};
  return name;
}

bool GPIO_MonitorTask::OnTaskEnable() {
  pinMode(IO_EXPANDER_RESET_CONTROL, OUTPUT);
  digitalWrite(IO_EXPANDER_RESET_CONTROL, HIGH);
  delay(100);

  ioExpanderA.reset();

  for (int portPin = 0; portPin < 16; portPin++) {
    ioExpanderA.pinMode(portPin, INPUT);
  }

  ioExpanderB.reset();

  for (int portPin = 0; portPin < 16; portPin++) {
    ioExpanderB.pinMode(portPin, INPUT);
  }

  lastExpanderAState = ioExpanderA.readGPIO();
  lastExpanderBState = ioExpanderB.readGPIO();

  return true;
}

void GPIO_MonitorTask::processExpanderDiff(uint16_t currentState,
                                           uint16_t& lastState,
                                           uint8_t pinOffset) {
  uint16_t stateDiff = currentState ^ lastState;

  if (stateDiff) {
    for (uint8_t i = 0; i < 16; i++) {
      if (stateDiff & (1 << i)) {
        String logEntry = "INPUT " + String(i + pinOffset) + "," +
                          ((currentState & (1 << i)) ? "OFF" : "ON");
        services_.getLoggingManager()->addLog(logEntry,
                                              String(currentState, BIN));
      }
    }
  }
  lastState = currentState;
}

bool GPIO_MonitorTask::TaskCallback() {
  uint16_t expanderAState, expanderBState;

  expanderAState = ioExpanderA.readGPIO();
  expanderBState = ioExpanderB.readGPIO();

  processExpanderDiff(expanderAState, lastExpanderAState, 0);
  processExpanderDiff(expanderBState, lastExpanderBState, 16);

  lastExpanderAState = expanderAState;
  lastExpanderBState = expanderBState;
  return true;
}

}  // namespace gpio_monitor
}  // namespace tasks
}  // namespace inamata