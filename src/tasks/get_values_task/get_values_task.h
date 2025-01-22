#pragma once

#include <ArduinoJson.h>

#include <memory>

#include "managers/service_getters.h"
#include "peripheral/capabilities/get_values.h"
#include "tasks/base_task.h"
#include "utils/uuid.h"

namespace inamata {
namespace tasks {
namespace get_values_task {

/**
 * Abstract class that implements getting a peripheral which supports the
 * GetValue capability for a given name.
 */
class GetValuesTask : public BaseTask {
 public:
  struct Input : public BaseTask::Input {
    virtual ~Input() = default;
    utils::UUID peripheral_id{nullptr};
    JsonDocument start_measurement_parameters;
  };

  GetValuesTask(const ServiceGetters& services, Scheduler& scheduler,
                const Input& input);
  virtual ~GetValuesTask() = default;

  std::shared_ptr<peripheral::capabilities::GetValues> getPeripheral();
  const utils::UUID& getPeripheralUUID() const;

  /**
   * @param[in] parameters The JSON object with the parameters
   * @param[out] input The parsed parameters used by the constructor
   */
  static void populateInput(const JsonObjectConst& parameters, Input& input);

  /**
   * Send a telemetry message with the given GetValues result
   *
   * \param result Output of getValues from a GetValues peripheral
   */
  void sendTelemetry(peripheral::capabilities::GetValues::Result& result);

  static const __FlashStringHelper* threshold_key_;
  static const __FlashStringHelper* threshold_key_error_;
  static const __FlashStringHelper* trigger_type_key_;
  static const __FlashStringHelper* trigger_type_key_error_;
  static const __FlashStringHelper* interval_ms_key_;
  static const __FlashStringHelper* interval_ms_key_error_;
  static const __FlashStringHelper* duration_ms_key_;

 protected:
  std::shared_ptr<peripheral::capabilities::GetValues> peripheral_;
  utils::UUID peripheral_id_;

 private:
  std::shared_ptr<WebSocket> web_socket_;
};

}  // namespace get_values_task
}  // namespace tasks
}  // namespace inamata
