#pragma once

#include <ArduinoJson.h>

#include <memory>

#include "managers/service_getters.h"
#include "peripheral/capabilities/set_value.h"
#include "tasks/base_task.h"

namespace inamata {
namespace tasks {
namespace set_value {

class SetValue : public BaseTask {
 public:
  SetValue(const ServiceGetters& services, const JsonObjectConst& parameters,
           Scheduler& scheduler);
  virtual ~SetValue() = default;

  const String& getType() const final;
  static const String& type();

  bool TaskCallback() final;

 private:
  static bool registered_;
  static BaseTask* factory(const ServiceGetters& services,
                           const JsonObjectConst& parameters,
                           Scheduler& scheduler);

  std::shared_ptr<peripheral::capabilities::SetValue> peripheral_;
  utils::UUID peripheral_id_;

  utils::ValueUnit value_unit_;

  std::shared_ptr<WebSocket> web_socket_;
  bool send_data_point_;

  static const __FlashStringHelper* send_data_point_key_;
  static const __FlashStringHelper* send_data_point_key_error_;
};

}  // namespace set_value
}  // namespace tasks
}  // namespace inamata
