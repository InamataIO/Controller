#include <Arduino.h>
#include <TaskScheduler.h>
#include <tinyexpr.h>

#include "managers/services.h"
#include "utils/setup_node.h"

//----------------------------------------------------------------------------
// Global instances
inamata::Services services;
Scheduler& scheduler = services.getScheduler();

//----------------------------------------------------------------------------
// Setup and loop functions
void setup() {
  bool success = inamata::setupNode(services);
  if (success) {
    TRACELN(F("Setup finished"));
  } else {
    TRACELN(F("Node setup failed. Restarting in 10s"));
    delay(10000);
    ESP.restart();
  }
}

bool called = false;

void loop() { scheduler.execute(); }
