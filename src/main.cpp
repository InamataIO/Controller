#include <Arduino.h>
#include <TaskScheduler.h>

#include "managers/services.h"
#include "tasks/factory_reset/factory_reset.h"
#include "utils/setup_node.h"

//----------------------------------------------------------------------------
// Global instances
inamata::Services services;
Scheduler& scheduler = services.getScheduler();
// Separate scheduler so recovery normal task failures ignored
Scheduler recovery_scheduler;
inamata::tasks::factory_reset::FactoryReset factory_reset(recovery_scheduler);

//----------------------------------------------------------------------------
// Setup and loop functions
void setup() {
  bool success = inamata::setupNode(services, factory_reset);
  if (success) {
    TRACELN("Setup finished");
  } else {
    inamata::restartAfterSetupFailure(recovery_scheduler, factory_reset);
  }
}

void loop() {
  recovery_scheduler.execute();
  scheduler.execute();
}
