#include "managers/action_controller.h"

namespace inamata {

void ActionController::setServices(ServiceGetters services) {
  services_ = services;
}

void ActionController::handleCallback(const JsonObjectConst& message) {
  JsonVariantConst action = message[WebSocket::action_key_];
  if (action.isNull()) {
    return;
  }

  if (action == action_restart_) {
    ESP.restart();
  } else if (action == action_clear_stored_resources_) {
    services_.getStorage()->deletePeripherals();
    ESP.restart();
  } else {
    TRACEF("Unknown action: %s\n", action);
  }
}

const __FlashStringHelper* ActionController::action_restart_ = FPSTR("rst");
const __FlashStringHelper* ActionController::action_clear_stored_resources_ =
    FPSTR("clrStrdRes");

}  // namespace inamata