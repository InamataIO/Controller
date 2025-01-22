#pragma once

#include <chrono>
#include <memory>
#include <vector>

#include "peripheral/peripherals/modbus/modbus_client_input.h"
#include "tasks/base_task.h"
#include "utils/value_unit.h"

namespace inamata {
namespace tasks {
namespace fixed {

class PollAbstract : public BaseTask {
 public:
  using ModbusClientInput = peripheral::peripherals::modbus::ModbusClientInput;

  PollAbstract(Scheduler& scheduler);
  virtual ~PollAbstract() = default;

 protected:
  /**
   * Call to automatically get data periodically.
   *
   * Uses the value in measurement_wait_ to wait between measurements. Calls
   * handleResult() on receiving data.
   */
  void handlePoll();

  /**
   * Starts a new remote sensor measurement
   *
   * \param now Time now of the steady clock. Syncs with handleMeasurement()
   */
  void startMeasurement(const std::chrono::steady_clock::time_point now);

  /**
   * Handler callback when once a measurement is started
   *
   * \param now Time now of the steady clock. Syncs with startMeasurement()
   */
  void handleMeasurement(const std::chrono::steady_clock::time_point now);

  /**
   * Resets variables once a measurement completes or fails
   *
   * \param now Now of steady clock. Syncs with start/handleMeasurement()
   */
  void markMeasurementDone(const std::chrono::steady_clock::time_point now,
                           const bool error = false);

  /**
   * Called once data is received. Implemented by inheriting classes
   *
   * \param values The float and DPTs of the received data
   */
  virtual void handleResult(std::vector<utils::ValueUnit>& values) = 0;

  virtual void onError();

  /// Pointer to modbus input peripheral to poll
  std::shared_ptr<ModbusClientInput> modbus_input_;
  /// Auto-wait when using handlePoll(). Derived classes can override value
  std::chrono::seconds measurement_wait_{60};
  /// When the last measurement completed or failed
  std::chrono::steady_clock::time_point last_measurement_;
  /// Whether a measurement is currently active
  bool is_measurement_running_ = false;
  /// Incremented on errors, set to zero on success
  uint32_t errors_since_last_success_ = 0;
};

}  // namespace fixed
}  // namespace tasks
}  // namespace inamata