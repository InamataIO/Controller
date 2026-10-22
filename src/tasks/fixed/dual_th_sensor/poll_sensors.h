#pragma once

#include <TaskSchedulerDeclarations.h>

#include <array>
#include <chrono>

#include "managers/services.h"
#include "peripheral/peripherals/sht2x_sensor/sht2x_sensor.h"

namespace inamata {
namespace tasks {
namespace fixed {

class PollSensors : public BaseTask {
 public:
  PollSensors(const ServiceGetters& services, Scheduler& scheduler);
  virtual ~PollSensors() = default;

  const String& getType() const final;
  static const String& type();
  bool TaskCallback();

 private:
  bool setFixedPeripherals();

  std::shared_ptr<WebSocket> web_socket_;
  struct SensorInfo {
    std::shared_ptr<peripheral::peripherals::sht2x_sensor::SHT2xSensor>
        peripheral;
    bool measurement_active = false;
    bool cycle_complete = false;
  };

  std::array<SensorInfo, 2> sensors_;
  static const std::chrono::seconds default_interval_;
};

}  // namespace fixed
}  // namespace tasks
}  // namespace inamata
