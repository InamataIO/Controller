#pragma once

#include <TaskSchedulerDeclarations.h>

#include <chrono>

#include "managers/services.h"
#include "peripheral/peripherals/hdc2080/hdc2080.h"
#include "peripheral/peripherals/sgp40/sgp40.h"

namespace inamata {
namespace tasks {
namespace fixed {

class CalibrateVoc : public BaseTask {
 public:
  CalibrateVoc(Scheduler& scheduler);
  virtual ~CalibrateVoc() = default;

  const String& getType() const final;
  static const String& type();

  bool TaskCallback();

 private:
  /**
   * Set the VOC and air (temp./humidity) sensor peripherals
   *
   * \return True on success
   */
  bool setFixedPeripherals();

  Scheduler& scheduler_;
  std::shared_ptr<WebSocket> web_socket_;

  /// Measures the VOC index of the air
  std::shared_ptr<peripheral::peripherals::sgp40::SGP40> voc_sensor_;
  /// Measures the temperature and humidity
  std::shared_ptr<peripheral::peripherals::hdc2080::HDC2080> air_sensor_;
  utils::UUID humidity_dpt_{nullptr};
  utils::UUID temperature_c_dpt_{nullptr};

  // Max time is ~72 minutes due to an overflow in the CPU load counter
  static const std::chrono::minutes default_interval_;
};

}  // namespace fixed
}  // namespace tasks
}  // namespace inamata