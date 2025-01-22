#pragma once

#include <memory>

#include "peripheral/peripherals/digital_out/digital_out.h"

namespace inamata {
namespace tasks {
namespace fixed {

class Leds {
 public:
  Leds();
  virtual ~Leds() = default;

  using DigitalOut = peripheral::peripherals::digital_out::DigitalOut;

 protected:
  String checkLeds();

  std::shared_ptr<DigitalOut> led_fault_;
  std::shared_ptr<DigitalOut> led_alarm_1_;
  std::shared_ptr<DigitalOut> led_alarm_2_;
  std::shared_ptr<DigitalOut> led_alarm_3_;
  std::shared_ptr<DigitalOut> led_alarm_4_;
  std::shared_ptr<DigitalOut> led_network_;
};

}  // namespace fixed
}  // namespace tasks
}  // namespace inamata