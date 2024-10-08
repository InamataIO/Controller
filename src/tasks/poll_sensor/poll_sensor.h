#pragma once

#include <ArduinoJson.h>

#include <memory>

#include "managers/service_getters.h"
#include "peripheral/capabilities/start_measurement.h"
#include "tasks/get_values_task/get_values_task.h"

namespace inamata {
namespace tasks {
namespace poll_sensor {

/**
 * Polls the sensor via the GetValues capability
 *
 * Uses the StartMeasurement capability if the sensor supports it. The interval
 * parameter is interpreted as the time after a reading has completed.
 * Therefore the total time between readings, if StartMeasurement is supported,
 * is the time to perform the measurement, read the values and then wait the
 * specified time interval.
 *
 * The duration parameter specifies for how long the sensor should be polled.
 * If a measurement has started before the duration ends, it will be completed
 * and sent to the server.
 */
class PollSensor : public get_values_task::GetValuesTask {
 public:
  struct Input : public GetValuesTask::Input {
    std::chrono::milliseconds interval{-1};
    std::chrono::milliseconds duration{-1};
  };

  PollSensor(const ServiceGetters& services, Scheduler& scheduler,
             const Input& input);
  virtual ~PollSensor() = default;

  const String& getType() const final;
  static const String& type();

  static void populateInput(const JsonObjectConst& parameters, Input& input);

  bool TaskCallback() final;

 private:
  static bool registered_;
  static BaseTask* factory(const ServiceGetters& services,
                           const JsonObjectConst& parameters,
                           Scheduler& scheduler);

  std::shared_ptr<WebSocket> web_socket_;

  /// Interval between readings
  std::chrono::milliseconds interval_;
  /// End the task after this time is reached
  std::chrono::steady_clock::time_point run_until_;
  std::shared_ptr<peripheral::capabilities::StartMeasurement>
      start_measurement_peripheral_ = nullptr;
};

}  // namespace poll_sensor
}  // namespace tasks
}  // namespace inamata
