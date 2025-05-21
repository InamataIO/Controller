#ifdef DEVICE_TYPE_TIAKI_CO2_MONITOR

#include "network_led.h"

#include "managers/services.h"
#include "peripheral/fixed.h"

namespace inamata {
namespace tasks {
namespace fixed {

NetworkLed::NetworkLed(const ServiceGetters& services, Scheduler& scheduler,
                       const JsonObjectConst& behavior_config)
    : BaseTask(scheduler, Input(nullptr, true)),
      led_dpt_(utils::UUID::fromFSH(peripheral::fixed::dpt_led_id)) {
  auto& peripheral_controller = Services::getPeripheralController();
  utils::UUID led_id =
      utils::UUID::fromFSH(peripheral::fixed::peripheral_led_network_id);
  network_led_ = std::dynamic_pointer_cast<DigitalOut>(
      peripheral_controller.getPeripheral(led_id));
  if (!network_led_) {
    setInvalid(ErrorStore::genNotAValid(led_id, DigitalOut::type()));
    return;
  }

  wifi_network_ = services.getWifiNetwork();
  if (wifi_network_ == nullptr) {
    setInvalid(services.wifi_network_nullptr_error_);
    return;
  }

  web_socket_ = services.getWebSocket();
  if (web_socket_ == nullptr) {
    setInvalid(services.web_socket_nullptr_error_);
    return;
  }
  web_socket_->setSentMessageCallback(
      std::bind(&NetworkLed::wsMessageSent, this));

  ble_server_ = services.getBleServer();
  if (ble_server_ == nullptr) {
    setInvalid(services.ble_server_nullptr_error_);
    return;
  }

  setIterations(TASK_FOREVER);
  enable();
}

NetworkLed::~NetworkLed() {
  if (web_socket_) {
    web_socket_->clearSentMessageCallback();
  }
}

const String& NetworkLed::getType() const { return type(); }

const String& NetworkLed::type() {
  static const String name{"NetworkLed"};
  return name;
}

bool NetworkLed::TaskCallback() {
  Task::delay(std::chrono::milliseconds(default_interval_).count());

  // Blink (off) when sending messages
  if (activity_blink_active_) {
    network_led_->clearOverride();
    activity_blink_active_ = false;
  }

  // Check if the WebSocket is connected
  const auto now = std::chrono::steady_clock::now();
  if (last_check_ + check_interval_ < now) {
    last_check_ = now;
    const LedState last_led_state = led_state_;
    updateLedState();
    if (last_led_state != led_state_) {
      last_blink_change_ = {};
      blink_pos_ = 0;
      TRACEF("State change: %d to %d\n", last_led_state, led_state_);
    }
  }

  // If no state, nothing to do until next check
  if (led_state_ == LedState::kNone) {
    Task::delay(std::chrono::milliseconds(check_interval_).count());
    network_led_->setValue(utils::ValueUnit(0, led_dpt_));
    return true;
  }

  // If WebSocket connected, leave LED on until next check
  if (led_state_ == LedState::kWebSocketConnected) {
    Task::delay(std::chrono::milliseconds(check_interval_).count());
    network_led_->setValue(utils::ValueUnit(1, led_dpt_));
    return true;
  }

  if (led_state_ == LedState::kWifiConnecting) {
    if (blink_pos_ >= wifi_connecting_blinks_.size()) {
      blink_pos_ = 0;
    }
    updateBlink(now,
                std::chrono::milliseconds(wifi_connecting_blinks_[blink_pos_]));
  } else if (led_state_ == LedState::kWebSocketConnecting) {
    if (blink_pos_ >= ws_connecting_blinks_.size()) {
      blink_pos_ = 0;
    }
    updateBlink(now,
                std::chrono::milliseconds(ws_connecting_blinks_[blink_pos_]));
  } else if (led_state_ == LedState::kImprov) {
    if (blink_pos_ >= improv_blinks_.size()) {
      blink_pos_ = 0;
    }
    updateBlink(now, std::chrono::milliseconds(improv_blinks_[blink_pos_]));
  }

  return true;
}

void NetworkLed::updateLedState() {
  const bool ws_connected = web_socket_->isConnected();
  WiFiNetwork::ConnectMode mode = wifi_network_->getMode();
  if (ws_connected) {
    led_state_ = LedState::kWebSocketConnected;
    return;
  }

  if (ble_server_->isActive()) {
    led_state_ = LedState::kImprov;
    return;
  }
  if (mode == WiFiNetwork::ConnectMode::kConnected ||
      mode == WiFiNetwork::ConnectMode::kFastConnect) {
    led_state_ = LedState::kWebSocketConnecting;
    return;
  }
  led_state_ = LedState::kWifiConnecting;
}

void NetworkLed::updateBlink(const std::chrono::steady_clock::time_point now,
                             const std::chrono::milliseconds blink_interval) {
  const auto diff = now - last_blink_change_;
  if (diff > blink_interval) {
    // Even are on, odd are off. Index (blink_pos_) starts at 0
    const bool led_on = blink_pos_ % 2;
    network_led_->setValue(utils::ValueUnit(led_on, led_dpt_));
    last_blink_change_ = now;
    blink_pos_++;
  }
}

void NetworkLed::wsMessageSent() {
  if (led_state_ == LedState::kWebSocketConnected) {
    Task::delay(std::chrono::milliseconds(default_interval_).count());
    network_led_->setOverride(false);
    activity_blink_active_ = true;
  }
}

const std::chrono::milliseconds NetworkLed::default_interval_{100};

}  // namespace fixed
}  // namespace tasks
}  // namespace inamata

#endif