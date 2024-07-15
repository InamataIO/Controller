#pragma once

#include <ArduinoJson.h>

#include <memory>

#include "managers/service_getters.h"
#include "peripheral/capabilities/led_strip.h"
#include "tasks/base_task.h"
#include "utils/color.h"
#include "utils/uuid.h"

namespace inamata {
namespace tasks {
namespace set_rgb_led {

class SetRgbLed : public BaseTask {
 public:
  /// Data used for local construction
  struct Input : public BaseTask::Input {
    virtual ~Input() = default;
    utils::UUID peripheral_id{nullptr};
    float brightness;
    utils::Color color;
  };

  SetRgbLed(Scheduler& scheduler, const Input& input);
  virtual ~SetRgbLed() = default;

  const String& getType() const final;
  static const String& type();

  static void populateInput(const JsonObjectConst& parameters, Input& input);

  bool TaskCallback() final;

 private:
  /**
   * Extract variant and return 0-255 value. -1 if invalid
   *
   * \param color JSON variant with the color as a number
   * \return Brightness of color in 0-255 range. -1 if invalid
   */
  static int toColor(JsonVariantConst color);

  static bool registered_;
  static BaseTask* factory(const ServiceGetters& services,
                           const JsonObjectConst& parameters,
                           Scheduler& scheduler);

  std::shared_ptr<peripheral::capabilities::LedStrip> peripheral_;
  utils::UUID peripheral_uuid_;

  static const __FlashStringHelper* rgb_key_;
  static const __FlashStringHelper* color_key_;
  static const __FlashStringHelper* brightness_key_;
  static const __FlashStringHelper* red_key_;
  static const __FlashStringHelper* green_key_;
  static const __FlashStringHelper* blue_key_;
  static const __FlashStringHelper* white_key_;
  static const __FlashStringHelper* color_error_;

  utils::Color color_;
};

}  // namespace set_rgb_led
}  // namespace tasks
}  // namespace inamata
