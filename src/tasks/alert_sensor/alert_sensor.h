#pragma once

#include <ArduinoJson.h>

#include <memory>

#include "managers/service_getters.h"
#include "tasks/get_values_task/get_values_task.h"
#include "utils/value_unit.h"

namespace inamata {
namespace tasks {
namespace alert_sensor {

class AlertSensor : public get_values_task::GetValuesTask {
  // TODO: Add start measurement support
 public:
  // Synced with trigger_type_names_
  enum class TriggerType {
    kInvalid = 0,
    kRising = 1,
    kFalling = 2,
    kEither = 3
  };

  struct Input : public GetValuesTask::Input {
    virtual ~Input() = default;
    /// Whether to trigger on rising, fallind or both edges
    TriggerType trigger_type = TriggerType::kInvalid;
    /// The threshold used to trigger on
    float threshold = NAN;
    /// When getting values from peripheral filter for this DPT
    utils::UUID data_point_type_id{nullptr};
    /// How often to get values from peripheral
    std::chrono::milliseconds interval{100};
    /// Stop task after this duration. -1 to run forever
    std::chrono::milliseconds duration{-1};
    /// Stop task after n triggers. -1 to ignore. AND when used with duration
    /// For 1, it'll stop after 1 trigger, for 2 it'll stop after 2 and so on
    int32_t trigger_count_limit{-1};
  };

  AlertSensor(const ServiceGetters& services, Scheduler& scheduler,
              const Input& input);
  virtual ~AlertSensor() = default;

  const String& getType() const final;
  static const String& type();

  static void populateInput(const JsonObjectConst& parameters, Input& input);

  bool TaskCallback() final;

  std::function<bool(TriggerType)> handle_output_;

 private:
  /**
   * Parses the trigger type
   *
   * Used to evaluate which type of crossing the threshold will cause an alert
   * to be sent. Sets kInvalid type on unknown type.
   *
   * @param trigger_type The type as string [rising, falling, either]
   * @return Trigger type, kInvalid on failure
   */
  static TriggerType toTriggerType(const char* trigger_type);

  static const __FlashStringHelper* fromTriggerType(
      const TriggerType trigger_type);

  bool sendAlert(TriggerType trigger_type);
  bool isRisingThreshold(const float value);
  bool isFallingThreshold(const float value);
  void incrementTriggerCount();

  static bool registered_;
  static BaseTask* factory(const ServiceGetters& services,
                           const JsonObjectConst& parameters,
                           Scheduler& scheduler);

  static const std::array<const __FlashStringHelper*, 3> trigger_type_name_;

  /// Interface to send data to the server
  std::shared_ptr<WebSocket> web_socket_;

  /// Trigger on rising, falling or both edge types
  TriggerType trigger_type_;

  /// The threshold to trigger on
  float threshold_;

  /// The data point type to filter for in the get_values array
  utils::UUID data_point_type_id_;

  /// Stop after n triggers
  int32_t trigger_count_limit_;
  /// The counter for trigger_count_limit_
  int32_t trigger_count_{0};

  /// Last measured sensor value
  float last_value_ = NAN;

  static const __FlashStringHelper* trigger_count_limit_key_;
  static const __FlashStringHelper* trigger_count_limit_key_error_;
};

}  // namespace alert_sensor
}  // namespace tasks
}  // namespace inamata
