#include "u8g2.h"

#include "managers/storage.h"
#include "peripheral/peripheral_factory.h"

namespace inamata {
namespace peripheral {
namespace peripherals {
namespace u8g2 {

U8G2::U8G2(const JsonObjectConst& parameters)
    : I2CAbstractPeripheral(parameters) {
  // If the base class constructor failed, abort the constructor
  if (!isValid()) {
    return;
  }

  const char* rotation_str = parameters["rotation"].as<const char*>();
  const u8g2_cb_t* rotation = U8G2_R0;
  if (rotation_str) {
    if (strcmp(rotation_str, "90") == 0) {
      rotation = U8G2_R1;
    } else if (strcmp(rotation_str, "180") == 0) {
      rotation = U8G2_R2;
    } else if (strcmp(rotation_str, "270") == 0) {
      rotation = U8G2_R3;
    } else if (strcmp(rotation_str, "hmirror") == 0) {
      rotation = U8G2_MIRROR;
    } else if (strcmp(rotation_str, "vmirror") == 0) {
      rotation = U8G2_MIRROR_VERTICAL;
    }
  }

  auto i2c_method = u8x8_byte_arduino_hw_i2c;
  if (getWireIndex() == 1) {
    i2c_method = u8x8_byte_arduino_2nd_hw_i2c;
  }

  JsonVariantConst display = parameters["display"];
  if (display == "SH1106") {
    // Note: Using default I2C address
    u8g2_Setup_sh1106_i2c_128x64_noname_f(
        driver_.getU8g2(), rotation, i2c_method, u8x8_gpio_and_delay_arduino);
    u8x8_SetPin_HW_I2C(driver_.getU8x8(), U8X8_PIN_NONE, U8X8_PIN_NONE,
                       U8X8_PIN_NONE);
  } else {
    const String error =
        String("Unknown display type") + display.as<const char*>();
    setInvalid(error);
    return;
  }
  driver_.begin();

  driver_.setFont(u8g2_font_6x10_tr);
  u8g2_uint_t y = driver_.getAscent() + 2;
  driver_.drawStr(2, y, Storage::device_type_name_ + 8);
  driver_.drawFrame(0, 0, 128, 64);
  driver_.sendBuffer();
}

const String& U8G2::getType() const { return type(); }

const String& U8G2::type() {
  static const String name{"U8G2"};
  return name;
}

::U8G2* U8G2::getDriver() { return &driver_; }

std::shared_ptr<Peripheral> U8G2::factory(const ServiceGetters& services,
                                          const JsonObjectConst& parameters) {
  return std::make_shared<U8G2>(parameters);
}

bool U8G2::registered_ = PeripheralFactory::registerFactory(type(), factory);

}  // namespace u8g2
}  // namespace peripherals
}  // namespace peripheral
}  // namespace inamata