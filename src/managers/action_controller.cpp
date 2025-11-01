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
    services_.getWebSocket()->disconnect();
    delay(500);
    ESP.restart();
  } else if (action == action_clear_stored_resources_) {
    services_.getStorage()->deletePeripherals();
    ESP.restart();
  } else if (action == action_identify_) {
    identify();
  } else {
    TRACEF("Unknown action: %s\r\n", action);
  }
}

void ActionController::identify() {
  if (identify_callback_) {
    identify_callback_();
  }
}

void ActionController::setIdentifyCallback(std::function<void()> callback) {
  identify_callback_ = callback;
}

void ActionController::clearIdentifyCallback() { identify_callback_ = nullptr; }

const __FlashStringHelper* ActionController::action_restart_ = FPSTR("rst");
const __FlashStringHelper* ActionController::action_clear_stored_resources_ =
    FPSTR("clrStrdRes");
const __FlashStringHelper* ActionController::action_identify_ = FPSTR("ident");

}  // namespace inamata