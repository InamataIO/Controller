#pragma once
#include <array>
#include <bitset>

#include "managers/services.h"
#include "peripheral/capabilities/get_values.h"
#include "peripheral/capabilities/start_measurement.h"
#include "peripheral/fixed.h"
#include "peripheral/peripheral.h"
#include "tasks/connectivity/connectivity.h"
#include "utils/uuid.h"

namespace inamata {
namespace tasks {
namespace fixed {

class Telemetry : public BaseTask {
 public:
  using StartMeasurement = peripheral::capabilities::StartMeasurement;
  using GetValues = peripheral::capabilities::GetValues;

  Telemetry(const ServiceGetters& services, Scheduler& scheduler);
  virtual ~Telemetry() = default;

  const String& getType() const final;
  static const String& type();

  bool TaskCallback();

 private:
  struct Sensor {
    enum class State {
      kIdle,
      kHandleMeasurement,
      kGetValues,
      kUpdated,
      kFailed,
    };

    std::shared_ptr<peripheral::Peripheral> peripheral;
    std::vector<utils::ValueUnit> values;
    State state = Sensor::State::kIdle;
    std::chrono::steady_clock::time_point wait_until =
        std::chrono::steady_clock::time_point::min();
  };
  std::vector<Sensor> sensors_;

  std::shared_ptr<WebSocket> web_socket_;

  std::chrono::steady_clock::time_point last_send_ =
      std::chrono::steady_clock::time_point::min();
  std::chrono::seconds send_interval_ = std::chrono::minutes(1);

  // Max time is ~72 minutes due to an overflow in the CPU load counter
  static const std::chrono::milliseconds default_interval_;
};

}  // namespace fixed
}  // namespace tasks
}  // namespace inamata