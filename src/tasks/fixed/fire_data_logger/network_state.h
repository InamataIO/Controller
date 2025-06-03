#pragma once

#include <TaskSchedulerDeclarations.h>

#include <chrono>

#include "managers/services.h"
#include "peripheral/peripherals/neo_pixel/neo_pixel.h"

namespace inamata {
namespace tasks {
namespace fixed {

class NetworkState : public BaseTask {
 public:
  NetworkState(const ServiceGetters& services, Scheduler& scheduler,
               const JsonObjectConst& behavior_config);
  virtual ~NetworkState() = default;

  const String& getType() const final;
  static const String& type();

  bool TaskCallback();

 private:
  enum class Mode { kDisplaySignalStrength, kDisplayProvisioning };

  /**
   * Set the VOC and RGB LED peripherals
   *
   * \return True on success
   */
  bool setFixedPeripherals();
  void handleDisplaySignalStrength();
  void handleDisplayProvisioning();

  Scheduler& scheduler_;
  std::shared_ptr<GsmNetwork> gsm_network_;
  std::shared_ptr<BleServer> ble_server_;

  Mode mode_ = Mode::kDisplaySignalStrength;
  int8_t signal_strength_blinks_ = 127;

  std::shared_ptr<peripheral::peripherals::neo_pixel::NeoPixel> status_led_;

  // Max time is ~72 minutes due to an overflow in the CPU load counter
  static const std::chrono::seconds default_interval_;

  uint32_t iterations_ = 0;
};

}  // namespace fixed
}  // namespace tasks
}  // namespace inamata