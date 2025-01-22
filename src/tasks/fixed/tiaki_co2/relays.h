#pragma once

#include <memory>

#include "peripheral/peripherals/digital_out/digital_out.h"
#include "peripheral/peripherals/gp8xxx/gp8xxx.h"

namespace inamata {
namespace tasks {
namespace fixed {

class Relays {
 public:
  Relays();
  virtual ~Relays() = default;

  using DigitalOut = peripheral::peripherals::digital_out::DigitalOut;
  using GP8XXX = peripheral::peripherals::gp8xxx::GP8XXX;

 protected:
  String checkRelays();

  std::shared_ptr<DigitalOut> buzzer_;
  std::shared_ptr<DigitalOut> relay_alarm_1_;
  std::shared_ptr<DigitalOut> relay_alarm_2_;
  std::shared_ptr<DigitalOut> relay_alarm_3_;
  std::shared_ptr<DigitalOut> relay_alarm_4_;
  std::shared_ptr<GP8XXX> analog_out_;
};

}  // namespace fixed
}  // namespace tasks
}  // namespace inamata