#pragma once

#include <functional>
#include <vector>

#include "managers/service_getters.h"

namespace inamata {

class BehaviorController {
 public:
  BehaviorController() = default;
  ~BehaviorController() = default;

  void setServices(ServiceGetters services);

  /**
   * Handle the command to update the firmware
   *
   * \param message Command with the controller action
   */
  void handleCallback(const JsonObjectConst& message);

  /**
   * Parse and handle behavior config (from WS CMD and local storage)
   *
   * \param message The behavior config
   */
  void handleConfig(const JsonObjectConst& message);

  void registerConfigCallback(
      std::function<void(const JsonObjectConst& message)> callback);

  void setRegisterData(JsonObject msg);

 private:
  utils::UUID behavior_id_{nullptr};
  uint64_t updated_at_{0};
  bool sent_fixed_peripherals_ = false;

  ServiceGetters services_;
  /// Callbacks that parse behavior configs
  std::vector<std::function<void(const JsonObjectConst& message)>> handlers_;

  static const __FlashStringHelper* set_command_key_;
  static const __FlashStringHelper* clear_command_key_;
  static const __FlashStringHelper* behavior_id_key_;
  static const __FlashStringHelper* updated_at_key_;
  static const __FlashStringHelper* device_id_key_;
};

}  // namespace inamata