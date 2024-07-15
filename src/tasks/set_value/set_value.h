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
  /// Data used for local construction
  struct Input : public BaseTask::Input {
    virtual ~Input() = default;
    utils::UUID peripheral_id{nullptr};
    utils::ValueUnit value_unit = utils::ValueUnit();
  };

  SetValue(Scheduler& scheduler, const Input& input);
  virtual ~SetValue() = default;

  const String& getType() const final;
  static const String& type();

  static void populateInput(const JsonObjectConst& parameters, Input& input);

  bool TaskCallback() final;

  void sendTelemetry(utils::ValueUnit value_unit,
                     const utils::UUID* lac_id = nullptr);

  /// Allows LACs to intercept the read data
  std::function<void(utils::ValueUnit&, SetValue&)> handle_output_;

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
