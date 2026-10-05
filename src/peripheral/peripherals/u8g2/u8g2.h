#pragma once

#include <ArduinoJson.h>
#include <U8g2lib.h>

#include "ArduinoJson/Object/JsonObjectConst.hpp"
#include "managers/service_getters.h"
#include "peripheral/peripheral.h"
#include "peripheral/peripherals/i2c/i2c_abstract_peripheral.h"

namespace inamata {
namespace peripheral {
namespace peripherals {
namespace u8g2 {

class U8G2 : public peripherals::i2c::I2CAbstractPeripheral {
 public:
  U8G2(const JsonObjectConst& parameters);
  virtual ~U8G2() = default;

  // Type registration in the peripheral factory
  const String& getType() const final;
  static const String& type();

  ::U8G2* getDriver();

 private:
  static std::shared_ptr<Peripheral> factory(const ServiceGetters& services,
                                             const JsonObjectConst& parameters);
  static bool registered_;

  ::U8G2 driver_;
};

}  // namespace u8g2
}  // namespace peripherals
}  // namespace peripheral
}  // namespace inamata