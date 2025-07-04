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
      if (isTimeSynced()) {
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
        if (isTimeSynced()) {
          handleWebSocket();
        }
      }
    }
  }
#endif
  else {
#ifdef PROV_WIFI
    if (now - mode_start_ > provision_timeout) {
      if (disable_captive_portal_timeout_) {
        TRACELN(F("Captive portal timeout reset"));
        mode_start_ = now;
      } else {
        TRACELN(F("Captive portal timed out"));
        setMode(Mode::ConnectWiFi);
      }
    }
    handleCaptivePortal();
#endif
#ifdef PROV_IMPROV
    if (now - mode_start_ > provision_timeout) {
      TRACELN(F("Improv setup timed out"));
      setMode(Mode::ConnectWiFi);
    } else {
      handleBleServer();
      handleImprov();
      if (improv_ && improv_->getState() == improv::STATE_STOPPED) {
        handleGsmWifiSwitch(now, true);
      }
    }
#endif
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

bool CheckConnectivity::isTimeSynced() {
  bool is_synced = wifi_network_->isTimeSynced();
#ifdef GSM_NETWORK
  if (!is_synced) {
    is_synced = gsm_network_->isTimeSynced();
  }
#endif
  return is_synced;
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
  input_bank_ = std::dynamic_pointer_cast<PCA9536D>(
      Services::getPeripheralController().getPeripheral(
          peripheral::fixed::peripheral_io_3_id));
  if (!input_bank_) {
    setInvalid(ErrorStore::genNotAValid(peripheral::fixed::peripheral_io_3_id,
                                        PCA9536D::type()));
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
    const auto result = input_bank_->getValues();
    for (const auto& value : result.values) {
      if (value.data_point_type == peripheral::fixed::dpt_gsm_wifi_toggle_id) {
        const bool use_wifi = value.value < 0.5;
        if (use_wifi && use_network_ != UseNetwork::kWifi) {
          TRACELN("Switch to WiFi");
          use_network_ = UseNetwork::kWifi;
          enterConnectMode();
        } else if (!use_wifi && use_network_ != UseNetwork::kGsm) {
          TRACELN("Switch to GSM");
          use_network_ = UseNetwork::kGsm;
          enterConnectMode();
        }
        break;
      }
    }
  }
#else
  enterConnectMode();
#endif
}

void CheckConnectivity::enterConnectMode() {
#ifdef DEVICE_TYPE_FIRE_DATA_LOGGER
  switch (use_network_) {
    case UseNetwork::kWifi:
      NetworkClient::Impl::disableGsm();
      gsm_network_->disable();
      NetworkClient::Impl::enableWifi();
      setMode(Mode::ConnectWiFi);
      break;
    case UseNetwork::kGsm:
      NetworkClient::Impl::disableWifi();
      gsm_network_->enable();
      NetworkClient::Impl::enableGsm(&gsm_network_->modem_);
      setMode(Mode::ConnectGsm);
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
      // Do not disable GSM network on provision as it blocks sending SMS
    case Mode::ProvisionDevice:
#ifdef PROV_IMPROV
      if (improv_) {
        improv_->stop();
      }
      improv_ = nullptr;
      ble_server_->disable();
#endif
#ifdef PROV_WIFI
      wifi_manager_ = nullptr;
      ws_token_parameter_ = nullptr;
      core_domain_parameter_ = nullptr;
      secure_url_parameter_ = nullptr;
#endif
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
#ifdef PROV_WIFI
    case Mode::ProvisionDevice:
      disable_captive_portal_timeout_ = false;
      break;
#endif
  }
}

#ifdef PROV_IMPROV
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
#endif

#ifdef PROV_WIFI
void CheckConnectivity::handleCaptivePortal() {
  if (!wifi_manager_) {
    setupCaptivePortal();
  }
  bool connected = wifi_manager_->process();
  if (connected) {
    TRACELN(F("Connected!"));
    setMode(Mode::ConnectWiFi);
  }
}

void CheckConnectivity::setupCaptivePortal() {
  TRACELN(F("Setting up captive portal"));
  wifi_manager_ = std::unique_ptr<WiFiManager>(new WiFiManager());
  wifi_manager_->setConfigPortalBlocking(false);
  wifi_manager_->setSaveConnectTimeout(
      std::chrono::duration_cast<std::chrono::seconds>(wifi_connect_timeout)
          .count());
  wifi_manager_->setSaveConfigCallback(
      std::bind(&CheckConnectivity::saveCaptivePortalWifi, this));
  wifi_manager_->setSaveParamsCallback(
      std::bind(&CheckConnectivity::saveCaptivePortalParameters, this));
  wifi_manager_->setPreOtaUpdateCallback(
      std::bind(&CheckConnectivity::preOtaUpdateCallback, this));

  // Add WebSocket token param. If set, use placeholder to avoid secret leakage
  String ws_token_state =
      web_socket_->isWsTokenSet() ? ws_token_placeholder_ : F("");
  ws_token_parameter_ = std::unique_ptr<WiFiManagerParameter>(
      new WiFiManagerParameter(Storage::ws_token_key_, Storage::ws_token_key_,
                               ws_token_state.c_str(), 64));
  wifi_manager_->addParameter(ws_token_parameter_.get());

  // Add core domain param. Set default or config loaded from secrets.json
  core_domain_parameter_ =
      std::unique_ptr<WiFiManagerParameter>(new WiFiManagerParameter(
          Storage::core_domain_key_, Storage::core_domain_key_,
          web_socket_->core_domain_.c_str(), 32));
  wifi_manager_->addParameter(core_domain_parameter_.get());

  // Add boolean secure url param. "y" is true, "n" is false
  const char* secure_url_state = web_socket_->secure_url_ ? "y" : "n";
  secure_url_parameter_ = std::unique_ptr<WiFiManagerParameter>(
      new WiFiManagerParameter(Storage::secure_url_key_, "Use TLS? y=yes, n=no",
                               secure_url_state, 1));
  wifi_manager_->addParameter(secure_url_parameter_.get());

  String ssid(wifi_portal_ssid);
  String password(wifi_portal_password);
  wifi_manager_->startConfigPortal(ssid.c_str(), password.c_str());
}

void CheckConnectivity::saveCaptivePortalWifi() {
  TRACELN(F("Save WiFi"));

  // Save the WiFi credentials by updating or adding the new AP
  String ssid = wifi_manager_->getWiFiSSID(false);
  String wifi_password = wifi_manager_->getWiFiPass(false);
  TRACEF("Connected to %s:%s\r\n", ssid.c_str(), wifi_password.c_str());
  bool saved = false;
  for (WiFiAP& wifi_ap : wifi_network_->wifi_aps_) {
    if (ssid == wifi_ap.ssid) {
      wifi_ap.password = wifi_password;
      saved = true;
      break;
    }
  }
  if (!saved) {
    wifi_network_->wifi_aps_.push_back(
        {.ssid = ssid, .password = wifi_password});
  }
  saved = false;

  // Load WiFi APs from FS and update or add new AP details
  JsonDocument secrets_doc;
  ErrorResult error = services_.getStorage()->loadSecrets(secrets_doc);
  if (error.isError()) {
    TRACELN(error.toString());
    return;
  }
  JsonObject secrets = secrets_doc.as<JsonObject>();
  JsonArray wifi_aps = secrets[F("wifi_aps")].to<JsonArray>();
  for (JsonObject wifi_ap : wifi_aps) {
    if (wifi_ap[F("ssid")] == ssid) {
      wifi_ap[F("password")] = wifi_password;
      saved = true;
      break;
    }
  }
  if (!saved) {
    JsonObject wifi_ap = wifi_aps.add<JsonObject>();
    wifi_ap[F("ssid")] = ssid;
    wifi_ap[F("password")] = wifi_password;
  }
  error = services_.getStorage()->storeSecrets(secrets);
  if (error.isError()) {
    TRACELN(error.toString());
  }
}

void CheckConnectivity::saveCaptivePortalParameters() {
  TRACELN(F("Save parameters"));
  JsonDocument secrets_doc;
  ErrorResult error = services_.getStorage()->loadSecrets(secrets_doc);
  if (error.isError()) {
    TRACELN(error.toString());
    return;
  }
  JsonObject secrets = secrets_doc.isNull() ? secrets_doc.to<JsonObject>()
                                            : secrets_doc.as<JsonObject>();

  // Save the other parameters
  WiFiManagerParameter** parameters = wifi_manager_->getParameters();
  for (int i = 0; i < wifi_manager_->getParametersCount(); i++) {
    WiFiManagerParameter* param = parameters[i];
    if (strcmp(param->getID(), Storage::ws_token_key_) == 0) {
      if (String(ws_token_placeholder_) != param->getValue()) {
        secrets[Storage::ws_token_key_] = param->getValue();
        web_socket_->setWsToken(param->getValue());
      }
    } else if (strcmp(param->getID(), WebSocket::core_domain_key_) == 0) {
      secrets[WebSocket::core_domain_key_] = param->getValue();
      web_socket_->core_domain_ = param->getValue();
    } else if (strcmp(param->getID(), WebSocket::secure_url_key_) == 0) {
      const char secure_url = *(param->getValue());
      if (secure_url == 'y' || secure_url == 'Y') {
        secrets[WebSocket::secure_url_key_] = true;
        web_socket_->secure_url_ = true;
      } else if (secure_url == 'n' || secure_url == 'N') {
        secrets[WebSocket::secure_url_key_] = false;
        web_socket_->secure_url_ = false;
      }
    }
  }

  // Save the input parameters to FS or EEPROM
  error = services_.getStorage()->storeSecrets(secrets);
  if (error.isError()) {
    TRACELN(error.toString());
  }
}

void CheckConnectivity::preOtaUpdateCallback() {
  disable_captive_portal_timeout_ = true;
}
#endif

#ifdef PROV_WIFI
const __FlashStringHelper* CheckConnectivity::ws_token_placeholder_ =
    FPSTR("<token set>");
#endif

}  // namespace connectivity
}  // namespace tasks
}  // namespace inamata
