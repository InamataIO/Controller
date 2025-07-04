#include "behavior_controller.h"

#include "peripheral/fixed.h"
#include "services.h"

namespace inamata {

void BehaviorController::setServices(ServiceGetters services) {
  services_ = services;
}

void BehaviorController::handleCallback(const JsonObjectConst& message) {
  JsonVariantConst behavior_command = message[WebSocket::behavior_key_];
  if (behavior_command.isNull()) {
    return;
  }

  JsonObjectConst set_behavior =
      behavior_command[set_command_key_].as<JsonObjectConst>();
  if (set_behavior) {
    services_.getStorage()->storeBehavior(set_behavior);
    handleConfig(set_behavior);
  }

  JsonObjectConst clear_behavior =
      behavior_command[clear_command_key_].as<JsonObjectConst>();
  if (clear_behavior) {
    services_.getStorage()->deleteBehavior();
  }
}

void BehaviorController::handleConfig(const JsonObjectConst& config) {
  behavior_id_ = config[behavior_id_key_].as<const char*>();
  updated_at_ = config[updated_at_key_].as<int>();
  for (const auto& handler : handlers_) {
    handler(config);
  }
  TRACEF("bid: %s, update: %lld\r\n", behavior_id_.toString().c_str(),
         updated_at_);
}

void BehaviorController::registerConfigCallback(
    std::function<void(const JsonObjectConst& message)> callback) {
  handlers_.push_back(callback);
}

void BehaviorController::setRegisterData(JsonObject msg) {
  msg[behavior_id_key_] = behavior_id_.toString();
  msg[updated_at_key_] = updated_at_;
  msg[device_id_key_] = services_.getStorage()->device_type_id_;
  // Send fixed peripherals once per power cycle. Reduce network/server load
  if (!sent_fixed_peripherals_) {
    sent_fixed_peripherals_ = true;
    peripheral::fixed::setRegisterFixedPeripherals(msg);
  }
}

const __FlashStringHelper* BehaviorController::set_command_key_ = FPSTR("set");
const __FlashStringHelper* BehaviorController::clear_command_key_ =
    FPSTR("clear");
const __FlashStringHelper* BehaviorController::behavior_id_key_ = FPSTR("bid");
const __FlashStringHelper* BehaviorController::updated_at_key_ =
    FPSTR("update");
const __FlashStringHelper* BehaviorController::device_id_key_ = FPSTR("device");

}  // namespace inamata