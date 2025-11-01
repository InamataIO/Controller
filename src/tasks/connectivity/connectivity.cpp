#include "connectivity.h"

#include "configuration.h"
#ifdef DEVICE_TYPE_FIRE_DATA_LOGGER
#include "managers/network_client_impl.h"
#include "peripheral/fixed.h"
#include "utils/error_store.h"
#endif

namespace inamata {
namespace tasks {
namespace connectivity {

CheckConnectivity::CheckConnectivity(const ServiceGetters& services,
                                     Scheduler& scheduler)
    : BaseTask(scheduler, Input(nullptr, true)),
      services_(services),
      ble_server_(services.getBleServer()),
      wifi_network_(services.getWifiNetwork()),
#ifdef GSM_NETWORK
      gsm_network_(services.getGsmNetwork()),
#endif
      web_socket_(services.getWebSocket()) {
  if (ble_server_ == nullptr) {
    setInvalid(services.ble_server_nullptr_error_);
    return;
  }
  if (wifi_network_ == nullptr) {
    setInvalid(services.wifi_network_nullptr_error_);
    return;
  }
#ifdef GSM_NETWORK
  if (gsm_network_ == nullptr) {
    setInvalid(services.gsm_network_nullptr_error_);
    return;
  }
#endif
  if (web_socket_ == nullptr) {
    setInvalid(services.web_socket_nullptr_error_);
    return;
  }
  const bool success = initGsmWifiSwitch();
  if (!success) {
    return;
  }
  Task::setIterations(TASK_FOREVER);
}

const String& CheckConnectivity::getType() const { return type(); }

const String& CheckConnectivity::type() {
  static const String name{"CheckConnectivity"};
  return name;
}

bool CheckConnectivity::OnTaskEnable() {
  // If the WebSocket token has not been set, jump directly to provisioning
  handleGsmWifiSwitch(std::chrono::steady_clock::now(), true);
  if (!web_socket_->isWsTokenSet()) {
    mode_ = Mode::ProvisionDevice;
  }

  return true;
}

bool CheckConnectivity::TaskCallback() {
  const auto now = std::chrono::steady_clock::now();
  handleGsmWifiSwitch(now);
  if (mode_ == Mode::ConnectWiFi) {
    WiFiNetwork::ConnectMode connect_mode = wifi_network_->connect();
    // Disable starting WiFi captive portal if the WebSocket connects once
    if (connect_mode == WiFiNetwork::ConnectMode::kPowerOff) {
      if (!web_socket_connected_since_boot_) {
        setMode(Mode::ProvisionDevice);
      } else {
        wifi_network_->setMode(WiFiNetwork::ConnectMode::kFastConnect);
      }
    } else if (connect_mode == WiFiNetwork::ConnectMode::kConnected) {
      handleClockSync(now);
      if (Services::is_time_synced_) {
        handleWebSocket();
      }
    }
  }
#ifdef GSM_NETWORK
  else if (mode_ == Mode::ConnectGsm) {
    if (!web_socket_->isWsTokenSet()) {
      setMode(Mode::ProvisionDevice);
    } else {
      gsm_network_->handleConnection();
      if (gsm_network_->isGprsConnected()) {
        handleClockSync(now);
        if (Services::is_time_synced_) {
          handleWebSocket();
        }
      }
    }
  }
#endif
  else {
    if (now - mode_start_ > provision_timeout) {
      TRACELN("Improv setup timed out");
      setMode(Mode::ConnectWiFi);
    } else {
      handleBleServer();
      handleImprov();
      if (improv_ && improv_->getState() == improv::STATE_STOPPED) {
#ifdef DEVICE_TYPE_FIRE_DATA_LOGGER
        handleGsmWifiSwitch(now, true);
#else
        enterConnectMode();
#endif
      }
    }
  }

  // Delay at end to ensure delay even if processing takes longer
  Task::delay(std::chrono::milliseconds(check_connectivity_period).count());
  return true;
}

void CheckConnectivity::handleClockSync(
    const std::chrono::steady_clock::time_point now) {
  if (utils::chrono_abs(now - last_time_check_) > time_check_period_) {
    last_time_check_ = now;
    if (mode_ == Mode::ConnectWiFi) {
      wifi_network_->initTimeSync();
    }
#ifdef GSM_NETWORK
    else if (mode_ == Mode::ConnectGsm) {
      gsm_network_->syncTime();
    }
#endif
  }
}

void CheckConnectivity::handleWebSocket() {
  WebSocket::ConnectState state = web_socket_->handle();
  if (state == WebSocket::ConnectState::kConnected) {
    web_socket_connected_since_boot_ = true;
  } else if (state == WebSocket::ConnectState::kFailed) {
    TRACELN(F("WS failed"));
    if (web_socket_connected_since_boot_) {
      web_socket_->resetConnectAttempt();
    } else {
      setMode(Mode::ProvisionDevice);
    }
  }
}

bool CheckConnectivity::initGsmWifiSwitch() {
#ifdef DEVICE_TYPE_FIRE_DATA_LOGGER
  gsm_wifi_toggle_ = std::dynamic_pointer_cast<DigitalIn>(
      Services::getPeripheralController().getPeripheral(
          peripheral::fixed::peripheral_gsm_wifi_toggle_id));
  if (!gsm_wifi_toggle_) {
    setInvalid(ErrorStore::genNotAValid(
        peripheral::fixed::peripheral_gsm_wifi_toggle_id, DigitalIn::type()));
    return false;
  }
#endif
  return true;
}

void CheckConnectivity::handleGsmWifiSwitch(
    const std::chrono::steady_clock::time_point now, bool force) {
#ifdef DEVICE_TYPE_FIRE_DATA_LOGGER
  if (force) {
    use_network_ = UseNetwork::kNone;
    last_gsm_wifi_switch_check_ = std::chrono::steady_clock::time_point::min();
  }
  if (utils::chrono_abs(now - last_gsm_wifi_switch_check_) >
      gsm_wifi_switch_check_period_) {
    last_gsm_wifi_switch_check_ = now;
    const bool use_gsm = gsm_wifi_toggle_->readState();
    if (!use_gsm && use_network_ != UseNetwork::kWifi) {
      TRACELN("Switch to WiFi");
      use_network_ = UseNetwork::kWifi;
      enterConnectMode();
    } else if (use_gsm && use_network_ != UseNetwork::kGsm) {
      TRACELN("Switch to GSM");
      use_network_ = UseNetwork::kGsm;
      enterConnectMode();
    }
  }
#endif
}

void CheckConnectivity::enterConnectMode() {
#ifdef DEVICE_TYPE_FIRE_DATA_LOGGER
  switch (use_network_) {
    case UseNetwork::kWifi:
      WebSocketsNetworkClient::Impl::disableGsm();
      gsm_network_->disable();
      WebSocketsNetworkClient::Impl::enableWifi();
      setMode(Mode::ConnectWiFi);
      break;
    case UseNetwork::kGsm:
      WebSocketsNetworkClient::Impl::disableWifi();
      gsm_network_->enable();
      WebSocketsNetworkClient::Impl::enableGsm(&gsm_network_->modem_);
      setMode(Mode::ConnectGsm);
    case UseNetwork::kNone:
      TRACELN("Illegal switch");
      break;
  }
#else
  setMode(Mode::ConnectWiFi);
#endif
}

void CheckConnectivity::setMode(Mode mode) {
  // No actions if not changing mode
  if (mode == mode_) {
    return;
  }

  // Actions to run when leaving mode
  switch (mode_) {
    case Mode::ProvisionDevice:
      if (improv_) {
        improv_->stop();
      }
      improv_ = nullptr;
      ble_server_->disable();
      break;
#ifdef GSM_NETWORK
    // Do not disable GSM network on provision as it blocks sending SMS
    case Mode::ConnectGsm:
#endif
    case Mode::ConnectWiFi:
      // Do nothing ATM
      break;
  }

  mode_ = mode;
  mode_start_ = std::chrono::steady_clock::now();

  // Actions to run when entering mode
  switch (mode_) {
    case Mode::ConnectWiFi:
      wifi_network_->setMode(WiFiNetwork::ConnectMode::kFastConnect);
      // If not reset, will fail on WS connection after being provisioned
      // and will require a reboot
      web_socket_->resetConnectAttempt();
      break;
#ifdef GSM_NETWORK
    case Mode::ConnectGsm:
      // If not reset, will fail on WS connection after being provisioned
      // and will require a reboot
      web_socket_->resetConnectAttempt();
      break;
#endif
    case Mode::ProvisionDevice:
      break;
  }
}

void CheckConnectivity::handleBleServer() { ble_server_->enable(); }

void CheckConnectivity::handleImprov() {
  if (!ble_server_->isActive()) {
    return;
  }
  if (!improv_) {
    improv_ = std::unique_ptr<BleImprov>(new BleImprov(services_));
  }
  improv_->handle();
}

}  // namespace connectivity
}  // namespace tasks
}  // namespace inamata
