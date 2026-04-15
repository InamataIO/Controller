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
  virtual ~NetworkState();

  const String& getType() const final;
  static const String& type();

  bool TaskCallback();

 private:
  enum class Mode { kDisplaySignalStrength, kDisplayProvisioning, kIdentify };

  /**
   * Set the VOC and RGB LED peripherals
   *
   * \return True on success
   */
  bool setFixedPeripherals();

  /**
   * Show the signal strength (Wifi/mobile) in blinks (1 - 4)
   */
  void handleDisplaySignalStrength();

  /**
   * Change status color to provisioning color (green)
   */
  void handleDisplayProvisioning();

  /**
   * Run identify blink sequence in identify_pattern_
   */
  void handleIdentify();

  /**
   * Callback to start identify sequence
   */
  void identify();

  Scheduler& scheduler_;
  std::shared_ptr<GsmNetwork> gsm_network_;
  std::shared_ptr<BleServer> ble_server_;

  Mode mode_ = Mode::kDisplaySignalStrength;
  int8_t signal_strength_blinks_ = 127;

  /// The identify blink steps. 0 = inactive, 1 = first step
  uint8_t identify_step_ = 0;
  static const std::chrono::milliseconds identify_step_duration_;
  static const std::bitset<18> identify_pattern_;

  std::shared_ptr<peripheral::peripherals::neo_pixel::NeoPixel> status_led_;

  // Max time is ~72 minutes due to an overflow in the CPU load counter
  static const std::chrono::seconds default_interval_;

  uint32_t iterations_ = 0;
};

}  // namespace fixed
}  // namespace tasks
}  // namespace inamata