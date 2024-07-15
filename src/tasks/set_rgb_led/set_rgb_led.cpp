#ifdef ESP32
#include "set_rgb_led.h"

#include "managers/services.h"
#include "tasks/task_factory.h"

namespace inamata {
namespace tasks {
namespace set_rgb_led {

SetRgbLed::SetRgbLed(Scheduler& scheduler, const Input& input)
    : BaseTask(scheduler, input) {
  if (!isValid()) {
    return;
  }

  // Get the UUID to later find the pointer to the peripheral object
  if (!input.peripheral_id.isValid()) {
    setInvalid(peripheral_key_error_);
    return;
  }

  // Search for the peripheral for the given name
  auto peripheral =
      Services::getPeripheralController().getPeripheral(input.peripheral_id);
  if (!peripheral) {
    setInvalid(
        peripheral::Peripheral::peripheralNotFoundError(input.peripheral_id));
    return;
  }

  // Check that the peripheral supports the LedStrip interface capability
  peripheral_ =
      std::dynamic_pointer_cast<peripheral::capabilities::LedStrip>(peripheral);
  if (!peripheral_) {
    setInvalid(peripheral::capabilities::LedStrip::invalidTypeError(
        input.peripheral_id, peripheral));
    return;
  }

  // Check the color to be set
  if (!input.color.is_valid_) {
    setInvalid(color_error_);
    return;
  }
  color_ = input.color;

  enable();
}

const String& SetRgbLed::getType() const { return type(); }

const String& SetRgbLed::type() {
  static const String name{"SetRgbLed"};
  return name;
}

void SetRgbLed::populateInput(const JsonObjectConst& parameters, Input& input) {
  BaseTask::populateInput(parameters, input);

  // Get the UUID to later find the pointer to the peripheral object
  JsonVariantConst peripheral_id = parameters[peripheral_key_];
  if (!peripheral_id.isNull()) {
    input.peripheral_id = peripheral_id;
  }

  // The color consists of red, green , blue and optionally white components
  JsonVariantConst rgb = parameters[rgb_key_];
  JsonVariantConst color = parameters[color_key_];
  JsonVariantConst brightness = parameters[brightness_key_];

  // Limit the brightness range from 0 to 1
  if (brightness.is<float>()) {
    float brightness_clipped =
        std::fmax(0, std::fmin(brightness.as<float>(), 1));
    input.color = utils::Color::fromBrightness(brightness_clipped);
  }

  // Parse hex color (#)
  if (color.is<const char*>()) {
    input.color = utils::Color::fromHex(rgb.as<const char*>());
  }

  // Parse color object #AD03EF (RGB) or #DEADBEEF (RGBW)
  if (!color.isNull()) {
    int color_red = toColor(color[red_key_]);
    if (color_red < 0) {
      return;
    }

    int color_green = toColor(color[green_key_]);
    if (color_green < 0) {
      return;
    }

    int color_blue = toColor(color[blue_key_]);
    if (color_blue < 0) {
      return;
    }

    JsonVariantConst color_white = color[white_key_];
    if (color_white.is<float>() && color_white >= 0 && color_white <= 255) {
      input.color = utils::Color::fromRgbw(color_red, color_green, color_blue,
                                           color_white);
    } else if (color_white.isNull()) {
      input.color = utils::Color::fromRgbw(color_red, color_green, color_blue);
    } else {
      return;
    }
  }
}

bool SetRgbLed::TaskCallback() {
  peripheral_->turnOn(color_);
  return false;
}

int SetRgbLed::toColor(JsonVariantConst color) {
  if (!color.is<float>()) {
    return -1;
  }
  int color_number = color.as<int>();
  if (color_number < 0 || color_number > 255) {
    return -1;
  }
  return color_number;
}

bool SetRgbLed::registered_ = TaskFactory::registerTask(type(), factory);

BaseTask* SetRgbLed::factory(const ServiceGetters& services,
                             const JsonObjectConst& parameters,
                             Scheduler& scheduler) {
  Input input;
  populateInput(parameters, input);
  return new SetRgbLed(scheduler, input);
}

const __FlashStringHelper* SetRgbLed::rgb_key_ = F("rgb");
const __FlashStringHelper* SetRgbLed::color_key_ = F("color");
const __FlashStringHelper* SetRgbLed::brightness_key_ = F("brightness");
const __FlashStringHelper* SetRgbLed::color_error_ = F("Failed parsing color");
const __FlashStringHelper* SetRgbLed::red_key_ = F("red");
const __FlashStringHelper* SetRgbLed::green_key_ = F("green");
const __FlashStringHelper* SetRgbLed::blue_key_ = F("blue");
const __FlashStringHelper* SetRgbLed::white_key_ = F("white");

}  // namespace set_rgb_led
}  // namespace tasks
}  // namespace inamata

#endif