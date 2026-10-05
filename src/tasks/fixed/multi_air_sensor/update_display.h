#pragma once

#include <ArduinoJson.h>

#include <chrono>
#include <memory>
#include <vector>

#include "managers/services.h"
#include "peripheral/capabilities/get_values.h"
#include "peripheral/capabilities/start_measurement.h"
#include "peripheral/peripheral.h"
#include "peripheral/peripherals/u8g2/u8g2.h"
#include "tasks/connectivity/connectivity.h"
#include "utils/uuid.h"

namespace inamata {
namespace tasks {
namespace fixed {

class UpdateDisplay : public BaseTask {
 public:
  using StartMeasurement = peripheral::capabilities::StartMeasurement;
  using GetValues = peripheral::capabilities::GetValues;
  using P_U8G2 = peripheral::peripherals::u8g2::U8G2;
  using CheckConnectivity = connectivity::CheckConnectivity;

  UpdateDisplay(const ServiceGetters& services, Scheduler& scheduler,
                const JsonObjectConst& behavior_config);
  virtual ~UpdateDisplay() = default;

  const String& getType() const final;
  static const String& type();

  bool TaskCallback();

  void handleBehaviorConfig(const JsonObjectConst& config);

 private:
  struct Sensor {
    enum class State { kIdle, kHandleMeasurement, kGetValues, kUpdated };

    std::shared_ptr<peripheral::Peripheral> peripheral;
    /// Optional DPT used if sensor returns multiple
    utils::UUID data_point_type{nullptr};
    String name;
    String unit;
    uint8_t decimals;
    float value = 0;
    State state = Sensor::State::kIdle;
    std::chrono::steady_clock::time_point wait_until =
        std::chrono::steady_clock::time_point::min();
  };

  JsonObjectConst findConfig(JsonArrayConst tasks_config);
  void updateDisplay();
  void drawStatusBar();
  void drawAgingMessage(uint16_t hours_remaining);

  std::shared_ptr<GsmNetwork> gsm_network_;
  std::vector<Sensor> sensors_;
  std::shared_ptr<P_U8G2> display_;

  uint8_t x_offset_ = 0;
  uint8_t y_offset_ = 0;
  bool update_ticker_ = false;

  std::chrono::steady_clock::time_point last_measurement_start_ =
      std::chrono::steady_clock::time_point::min();
  static constexpr std::chrono::seconds measurement_interval_{2};

  /// Last time update_ticker_ was flipped, decoupled from redraw frequency
  std::chrono::steady_clock::time_point last_ticker_toggle_ =
      std::chrono::steady_clock::time_point::min();
  static constexpr std::chrono::milliseconds ticker_interval_{1000};

  /// Whether the aging message is shown instead of the status bar
  bool show_aging_message_ = false;
  std::chrono::steady_clock::time_point last_aging_toggle_ =
      std::chrono::steady_clock::time_point::min();
  static constexpr std::chrono::seconds aging_toggle_interval_{5};

  /// Max time is ~72 minutes due to an overflow in the CPU load counter
  static constexpr std::chrono::milliseconds default_interval_{250};
};

}  // namespace fixed
}  // namespace tasks
}  // namespace inamata