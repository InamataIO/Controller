#pragma once

#include <ArduinoJson.h>
#include <FS.h>

#include "LittleFS.h"
#include "managers/services.h"
#include "managers/types.h"
#include "tasks/factory_reset/factory_reset.h"
#include "utils/error_store.h"

namespace inamata {

bool setupNode(Services& services,
               tasks::factory_reset::FactoryReset& factory_reset);

void restartAfterSetupFailure(
    Scheduler& recovery_scheduler,
    const tasks::factory_reset::FactoryReset& factory_reset);

}  // namespace inamata
