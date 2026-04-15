#include "managers/action_controller.h"

namespace inamata {

void ActionController::setServices(ServiceGetters services) {
  services_ = services;
}

void ActionController::handleCallback(const JsonObjectConst& message) {
  JsonVariantConst action = message[WebSocket::action_key_];
  if (!action.isNull()) {
    if (action == action_restart_) {
      services_.getWebSocket()->disconnect();
      delay(500);
      ESP.restart();
    } else if (action == action_clear_stored_resources_) {
      services_.getStorage()->deletePeripherals();
      ESP.restart();
    } else if (action == action_factory_reset_) {
      services_.getStorage()->recursiveRm("/");
      ESP.restart();
    } else if (action == action_identify_) {
      identify();
    } else {
      TRACEF("Unknown action: %s\r\n", action);
    }
  }

  JsonVariantConst actions = message[WebSocket::actions_key_];
#ifdef GSM_NETWORK
  if (!actions.isNull()) {
    handleSetAllowedMnos(actions);
  }
#endif
}

#ifdef GSM_NETWORK
void ActionController::handleSetAllowedMnos(JsonObjectConst actions) {
  JsonObjectConst set_allowed_mnos =
      actions[set_allowed_mnos_key_].as<JsonObjectConst>();
  if (set_allowed_mnos.isNull()) {
    return;
  }

  JsonArrayConst allowed_mnos = set_allowed_mnos["mnos"].as<JsonArrayConst>();
  if (allowed_mnos.isNull()) {
    return;
  }

  std::vector<String> mnos;
  for (JsonVariantConst mno : allowed_mnos) {
    if (!mno.is<const char*>()) {
      return;
    }
    mnos.push_back(mno.as<const char*>());
  }

  ErrorResult result =
      services_.getGsmNetwork()->setAllowedMobileOperators(mnos);
  if (result.isError()) {
    TRACEF("Failed setting allowed MNOs: %s\r\n", result.toString().c_str());
  }
}
#endif

void ActionController::identify() {
  if (identify_callback_) {
    identify_callback_();
  }
}

void ActionController::setIdentifyCallback(std::function<void()> callback) {
  identify_callback_ = callback;
}

void ActionController::clearIdentifyCallback() { identify_callback_ = nullptr; }

const char* ActionController::action_restart_ = "rst";
const char* ActionController::action_clear_stored_resources_ = "clrStrdRes";
const char* ActionController::action_factory_reset_ = "factoryReset";
const char* ActionController::action_identify_ = "ident";
const char* ActionController::set_allowed_mnos_key_ = "setAllowedMnos";

}  // namespace inamata