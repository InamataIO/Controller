#pragma once

#include "managers/service_getters.h"

namespace inamata {

class ActionController {
 public:
  ActionController() = default;
  ~ActionController() = default;

  void setServices(ServiceGetters services);

  /**
   * Handle the command to update the firmware
   *
   * \param message Command with the controller action
   */
  void handleCallback(const JsonObjectConst& message);

  void identify();
  void setIdentifyCallback(std::function<void()> callback);
  void clearIdentifyCallback();

 private:
  /// The server to which to reply to
  ServiceGetters services_;

  std::function<void()> identify_callback_;

  static const __FlashStringHelper* action_restart_;
  static const __FlashStringHelper* action_clear_stored_resources_;
  static const __FlashStringHelper* action_identify_;
};

}  // namespace inamata