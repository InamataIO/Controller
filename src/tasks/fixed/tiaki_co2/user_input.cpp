#ifdef DEVICE_TYPE_TIAKI_CO2_MONITOR

#include "user_input.h"

#include "managers/services.h"
#include "peripheral/fixed.h"

namespace inamata {
namespace tasks {
namespace fixed {

UserInput::UserInput(const ServiceGetters& services, Scheduler& scheduler,
                     const JsonObjectConst& behavior_config)
    : PollAbstract(scheduler) {
  if (!isValid()) {
    return;
  }
  auto& peripheral_controller = Services::getPeripheralController();
  touch_1_ =
      std::dynamic_pointer_cast<DigitalIn>(peripheral_controller.getPeripheral(
          utils::UUID::fromFSH(peripheral::fixed::peripheral_touch_1_id)));
  touch_2_ =
      std::dynamic_pointer_cast<DigitalIn>(peripheral_controller.getPeripheral(
          utils::UUID::fromFSH(peripheral::fixed::peripheral_touch_2_id)));

  // TODO: Check if touch_1/_2 are nullptr
  Services::getActionController().setIdentifyCallback(
      std::bind(&UserInput::identify, this));

  web_socket_ = services.getWebSocket();
  if (web_socket_ == nullptr) {
    setInvalid(services.web_socket_nullptr_error_);
    return;
  }

  setIterations(TASK_FOREVER);
  enable();
}

UserInput::~UserInput() {
  Services::getActionController().clearIdentifyCallback();
}

const String& UserInput::getType() const { return type(); }

const String& UserInput::type() {
  static const String name{"UserInput"};
  return name;
}

bool UserInput::TaskCallback() {
  Task::delay(std::chrono::milliseconds(default_interval_).count());

  touch_1_->update();
  touch_2_->update();

  const auto now = std::chrono::steady_clock::now();

  // If true, buzzer and LED should beep and this stops the short beep
  if (clear_buzzer_next_ > 0) {
    clear_buzzer_next_--;
    if (clear_buzzer_next_ == 0) {
      buzzer_->clearOverride();
      led_alarm_1_->clearOverride();
      led_alarm_2_->clearOverride();
    }
  }
  // Show button 1 was pressed with a beep and LED blink. Stop on next iteration
  if (touch_1_->rose()) {
    clear_buzzer_next_ = 2;
    buzzer_->setOverride(true);
    led_alarm_1_->setOverride(!led_alarm_1_->getState());
    if (!is_measurement_running_) {
      startMeasurement(now);
    }
  }
  // Show button 2 was pressed with a beep and LED blink. Stop on next iteration
  if (touch_2_->rose()) {
    // Pressing button 2 during the relay test aborts it
    abortRelayTest();

    clear_buzzer_next_ = 2;
    buzzer_->setOverride(true);
    led_alarm_2_->setOverride(!led_alarm_2_->getState());
    startBlinkMode();
  }

  if (identify_start_.time_since_epoch().count()) {
    blink_mode_ = 2;
    if (identify_start_ + std::chrono::seconds(2) <
        std::chrono::steady_clock::now()) {
      abortBlinkMode();
      identify_start_ = {};
    }
  }

  handleBlinkMode();
  handleRelayTest();
  if (is_measurement_running_) {
    handleMeasurement(now);
  }

  return true;
}

void UserInput::startBlinkMode() {
  last_blink_change_ = std::chrono::steady_clock::now();
  blink_mode_ = 1;
}

void UserInput::abortBlinkMode() {
  blink_led_state_ = false;
  blink_mode_ = 0;
  buzz_with_blink_ = false;
  clearOverrides();
}

void UserInput::handleBlinkMode() {
  // Guard sections with blink_mode check as previous sections can disable it

  // Handles long button press to start relay test. Blinks LEDs with
  // increasing frequency. Only run if blink mode is active
  if (touch_2_->read() && blink_mode_) {
    if (touch_2_->currentDuration() < 2000) {
      if (blink_mode_ != 1) {
        blink_mode_ = 1;
      }
    } else if (touch_2_->currentDuration() < 4000) {
      if (blink_mode_ != 2) {
        blink_mode_ = 2;
      }
    } else if (touch_2_->currentDuration() < 6000) {
      if (blink_mode_ != 4) {
        blink_mode_ = 4;
      }
    } else {
      abortBlinkMode();
      startRelayTest();
    }
  }

  // When letting go of button 2, end blink mode
  if (touch_2_->fell() && blink_mode_) {
    abortBlinkMode();
  }

  if (blink_mode_) {
    // Toggle LEDs with frequency based on blink mode
    const std::chrono::milliseconds blink_interval =
        std::chrono::milliseconds(250) / blink_mode_;
    const std::chrono::steady_clock::time_point now =
        std::chrono::steady_clock::now();
    if (now - last_blink_change_ >= blink_interval) {
      last_blink_change_ = now;
      blink_led_state_ = !blink_led_state_;
      led_alarm_1_->setOverride(blink_led_state_);
      led_alarm_2_->setOverride(blink_led_state_);
      led_alarm_3_->setOverride(blink_led_state_);
      led_alarm_4_->setOverride(blink_led_state_);
      if (buzz_with_blink_) {
        buzzer_->setOverride(blink_led_state_);
      }
    }
  }
}

void UserInput::startRelayTest() {
  relay_test_mode_ = 1;
  last_relay_test_mode_change_ = std::chrono::steady_clock::now();
  led_alarm_1_->setOverride(true);
  relay_alarm_1_->setOverride(true);

  // Set overrides to all LEDs/relays as they may already be active
  led_alarm_2_->setOverride(false);
  relay_alarm_2_->setOverride(false);
  led_alarm_3_->setOverride(false);
  relay_alarm_3_->setOverride(false);
  led_alarm_4_->setOverride(false);
  relay_alarm_4_->setOverride(false);
}

void UserInput::abortRelayTest() {
  if (!relay_test_mode_) {
    return;
  }
  relay_test_mode_ = 0;
  clearOverrides();
}

void UserInput::clearOverrides() {
  led_fault_->clearOverride();
  led_alarm_1_->clearOverride();
  led_alarm_2_->clearOverride();
  led_alarm_3_->clearOverride();
  led_alarm_4_->clearOverride();

  relay_alarm_1_->clearOverride();
  relay_alarm_2_->clearOverride();
  relay_alarm_3_->clearOverride();
  relay_alarm_4_->clearOverride();

  analog_out_->clearOverride();
}

void UserInput::handleRelayTest() {
  if (!relay_test_mode_) {
    return;
  }
  const std::chrono::steady_clock::time_point now =
      std::chrono::steady_clock::now();
  if (now - last_relay_test_mode_change_ >= relay_test_mode_duration_) {
    relay_test_mode_++;
    last_relay_test_mode_change_ = now;
  } else {
    return;
  }

  switch (relay_test_mode_) {
    case 2:
      led_alarm_1_->setOverride(false);
      relay_alarm_1_->setOverride(false);
      led_alarm_2_->setOverride(true);
      relay_alarm_2_->setOverride(true);
      break;
    case 3:
      led_alarm_2_->setOverride(false);
      relay_alarm_2_->setOverride(false);
      led_alarm_3_->setOverride(true);
      relay_alarm_3_->setOverride(true);
      break;
    case 4:
      led_alarm_3_->setOverride(false);
      relay_alarm_3_->setOverride(false);
      led_alarm_4_->setOverride(true);
      relay_alarm_4_->setOverride(true);
      break;
    case 5:
      led_alarm_4_->setOverride(false);
      relay_alarm_4_->setOverride(false);
      led_fault_->setOverride(true);
      analog_out_->setOverride(0);
      break;
    case 6:
      led_fault_->setOverride(false);
      led_alarm_1_->setOverride(true);
      analog_out_->setOverride(0.004);
      break;
    case 7:
      led_alarm_2_->setOverride(true);
      analog_out_->setOverride(0.01);
      break;
    case 8:
      led_alarm_3_->setOverride(true);
      analog_out_->setOverride(0.015);
      break;
    case 9:
      led_alarm_4_->setOverride(true);
      analog_out_->setOverride(0.020);
      break;
    default:
      abortRelayTest();
      return;
  }
}

void UserInput::identify() {
  identify_start_ = std::chrono::steady_clock::now();
  buzz_with_blink_ = true;
}

void UserInput::handleResult(std::vector<utils::ValueUnit>& values) {
  JsonDocument doc_out;
  JsonObject result_object = doc_out.to<JsonObject>();
  web_socket_->packageTelemetry(values, modbus_input_->id, true, result_object);
  web_socket_->sendTelemetry(result_object);
}

const std::chrono::milliseconds UserInput::default_interval_{50};

}  // namespace fixed
}  // namespace tasks
}  // namespace inamata

#endif