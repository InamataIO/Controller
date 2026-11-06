#pragma once

#include <TaskSchedulerDeclarations.h>

#include <bitset>
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
   * Set the shared RGB status LED peripheral
   *
   * \return True on success
   */
  bool setFixedPeripherals();

  /**
   * Show signal strength (WiFi) in off blinks (1 - 4)
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

  std::shared_ptr<BleServer> ble_server_;

  Mode mode_ = Mode::kDisplaySignalStrength;
  int8_t signal_strength_blinks_ = 127;

  bool provision_blink_state_ = false;

  /// The identify blink steps. 0 = inactive, 1 = first step
  uint8_t identify_step_ = 0;
  static const std::chrono::milliseconds identify_step_duration_;
  static const std::bitset<18> identify_pattern_;

  std::shared_ptr<peripheral::peripherals::neo_pixel::NeoPixel> status_led_;

  static const std::chrono::seconds default_interval_;
};

}  // namespace fixed
}  // namespace tasks
}  // namespace inamata
