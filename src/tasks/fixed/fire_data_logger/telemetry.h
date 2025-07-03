#pragma once
#include <array>
#include <bitset>

#include "managers/services.h"
#include "peripheral/fixed.h"
#include "peripheral/peripherals/digital_in/digital_in.h"
#include "peripheral/peripherals/digital_out/digital_out.h"
#include "peripheral/peripherals/neo_pixel/neo_pixel.h"
#include "peripheral/peripherals/pca9536d/pca9536d.h"
#include "peripheral/peripherals/pca9539/pca9539.h"
#include "utils/uuid.h"

namespace inamata {
namespace tasks {
namespace fixed {

constexpr size_t kInputCount = 40;

class Telemetry : public BaseTask {
 public:
  using DigitalIn = peripheral::peripherals::digital_in::DigitalIn;
  using PCA9539 = peripheral::peripherals::pca9539::PCA9539;

  Telemetry(const ServiceGetters& services, Scheduler& scheduler);
  virtual ~Telemetry() = default;

  const String& getType() const final;
  static const String& type();

  bool TaskCallback();

 private:
  /**
   * Handle value from digital input
   *
   * \param value Measured input value
   * \param current_states Current state of all inputs
   */
  void saveResult(const utils::ValueUnit& value,
                  std::bitset<kInputCount>& current_states);

  void sendTelemetry(const std::bitset<kInputCount>& current_states,
                     const std::bitset<kInputCount>& diff);

  std::shared_ptr<WebSocket> web_socket_;

  std::shared_ptr<PCA9539> input_bank_1_;
  std::shared_ptr<PCA9539> input_bank_2_;
  std::array<std::shared_ptr<DigitalIn>, 8> input_bank_3_;

  std::bitset<kInputCount> previous_states_;
  static const std::array<const utils::UUID*, kInputCount> dpts_;

  std::chrono::steady_clock::time_point last_full_send_ =
      std::chrono::steady_clock::time_point::min();
  std::chrono::seconds full_send_period_ = std::chrono::hours(1);

  // Max time is ~72 minutes due to an overflow in the CPU load counter
  static const std::chrono::milliseconds default_interval_;
};

}  // namespace fixed
}  // namespace tasks
}  // namespace inamata