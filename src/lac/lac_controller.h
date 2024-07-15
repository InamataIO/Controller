#pragma once

#include <TaskSchedulerDeclarations.h>

#include "lac/local_action_chain.h"
#include "managers/service_getters.h"

namespace inamata {
namespace lac {

class LacController {
 public:
  LacController(Scheduler& scheduler);
  virtual ~LacController() = default;

  const String& type();

  void setServices(ServiceGetters services);

  /**
   * Callback for messages regarding LAC from the server
   *
   * @param message The message as a JSON doc
   */
  void handleCallback(const JsonObjectConst& message);

  static const __FlashStringHelper* lac_command_key_;

 private:
  LacErrorResult startLac(const ServiceGetters& services,
                          const JsonObjectConst& parameters);

  Scheduler& scheduler_;
  ServiceGetters services_;
};

}  // namespace lac
}  // namespace inamata