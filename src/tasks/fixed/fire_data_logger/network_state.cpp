#ifdef DEVICE_TYPE_FIRE_DATA_LOGGER

#include "network_state.h"

#include "managers/gsm_network.h"
#include "peripheral/fixed.h"

namespace inamata {
namespace tasks {
namespace fixed {

NetworkState::NetworkState(const ServiceGetters& services, Scheduler& scheduler,
                           const JsonObjectConst& behavior_config)
    : BaseTask(scheduler, Input(nullptr, true)),
      scheduler_(scheduler),
      gsm_network_(services.getGsmNetwork()),
      ble_server_(services.getBleServer()) {
  if (!isValid()) {
    return;
  }

  if (gsm_network_ == nullptr) {
    setInvalid(services.gsm_network_nullptr_error_);
    return;
  }
  if (ble_server_ == nullptr) {
    setInvalid(services.ble_server_nullptr_error_);
    return;
  }

  bool success = setFixedPeripherals();
  if (!success) {
    return;
  }

  setIterations(TASK_FOREVER);
  enable();
}

const String& NetworkState::getType() const { return type(); }

const String& NetworkState::type() {
  static const String name{"NetworkState"};
  return name;
}

bool NetworkState::TaskCallback() {
  Task::delay(std::chrono::milliseconds(default_interval_).count());

  if (ble_server_->isActive()) {
    mode_ = Mode::kDisplayProvisioning;
  } else {
    if (mode_ != Mode::kDisplaySignalStrength) {
      signal_strength_blinks_ = 127;
    }
    mode_ = Mode::kDisplaySignalStrength;
  }

  switch (mode_) {
    case Mode::kDisplaySignalStrength:
      handleDisplaySignalStrength();
      break;
    case Mode::kDisplayProvisioning:
      handleDisplayProvisioning();
      break;
  }

  return true;
}

bool NetworkState::setFixedPeripherals() {
  std::shared_ptr<peripheral::Peripheral> peripheral =
      Services::getPeripheralController().getPeripheral(
          peripheral::fixed::peripheral_status_led_id);
  if (!peripheral) {
    setInvalid(peripheral::Peripheral::peripheralNotFoundError(
        peripheral::fixed::peripheral_status_led_id));
    return false;
  }

  // Since the UUID is specified externally, check the type
  const String& neopixel_type =
      peripheral::peripherals::neo_pixel::NeoPixel::type();
  if (peripheral->getType() == neopixel_type && peripheral->isValid()) {
    status_led_ =
        std::static_pointer_cast<peripheral::peripherals::neo_pixel::NeoPixel>(
            peripheral);
  } else {
    setInvalid(peripheral::Peripheral::notAValidError(
        peripheral::fixed::peripheral_status_led_id, neopixel_type));
    return false;
  }
  return true;
}

void NetworkState::handleDisplaySignalStrength() {
  if (signal_strength_blinks_ == 127) {
    int8_t signal_quality;
    if (gsm_network_->isEnabled()) {
      // Range is 0 to 32
      signal_quality = gsm_network_->signal_quality_;
      // Connected so show signal quality in 1 to 4 blinks
      if (signal_quality) {
        // Map 1 to 32 to 1 to 4
        signal_strength_blinks_ = (signal_quality - 1) / 8 + 1;
      } else {
        // No signal so one long blink
        signal_strength_blinks_ = -1;
      }
    } else {
      // Handle WiFi signal strength
      if (WiFi.isConnected()) {
        // Range is -90 to -30
        signal_quality = WiFi.RSSI();
        if (signal_quality < -90) {
          signal_strength_blinks_ = -1;
        } else if (signal_quality < -75) {
          signal_strength_blinks_ = 1;
        } else if (signal_quality < -67) {
          signal_strength_blinks_ = 2;
        } else if (signal_quality < -55) {
          signal_strength_blinks_ = 3;
        } else {
          signal_strength_blinks_ = 4;
        }

      } else {
        // Not connected so one long blink
        signal_strength_blinks_ = -1;
      }
    }
    signal_strength_blinks_ *= 2;
  }

  unsigned long delay_ms = 0;
  if (signal_strength_blinks_ > 0) {
    // Positive is short blinks
    if (signal_strength_blinks_ % 2) {
      // Odd number means off part of blink
      status_led_->turnOff();
      delay_ms = 400;
    } else {
      // Even number means on part of blink
      status_led_->turnOn(utils::Color::fromRgbw(0, 0, 55, 0));
      delay_ms = 400;
    }
    signal_strength_blinks_--;
  } else if (signal_strength_blinks_ < 0) {
    // Negative means long blinks
    if (signal_strength_blinks_ % 2) {
      // Odd number means off part of blink
      status_led_->turnOff();
      delay_ms = 400;
    } else {
      // Even number means on part of blink
      status_led_->turnOn(utils::Color::fromRgbw(55, 0, 55, 0));
      delay_ms = 2000;
    }
    signal_strength_blinks_++;
  } else {
    // Zero is wait before repeating pattern
    signal_strength_blinks_ = 127;
    status_led_->turnOff();
    delay_ms = 2000;
  }

  if (delay_ms == 0) {
    TRACELN("No delay. Using 1000ms");
    delay_ms = 1000;
  }
  Task::delay(delay_ms);
}

void NetworkState::handleDisplayProvisioning() {
  status_led_->turnOn(utils::Color::fromRgbw(0, 55, 0, 0));
  Task::delay(1000);
}

const std::chrono::seconds NetworkState::default_interval_{2};

}  // namespace fixed
}  // namespace tasks
}  // namespace inamata

#endif