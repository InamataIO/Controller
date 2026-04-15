#include "ble_improv.h"

#include <yuarel.h>

namespace inamata {

BleImprov::BleImprov(ServiceGetters services) : services_(services) {}

void BleImprov::handle() {
  // If the BLE server is not running disable improv
  if (!services_.getBleServer()->isActive()) {
    state_ = improv::STATE_STOPPED;
    rpc_data_.clear();
    user_data_ = "";
    return;
  }
  // If the BLE Improv service does not exist, set it up
  if (ble_improv_service_ == nullptr) {
    setupService();
  }

  // Handle active commands
  handleWiFiConnectTimeout();
  handleGetWifiNetworks();
#ifdef GSM_NETWORK
  handleGetMobileOperators();
#endif

  // Handle incoming RPC data
  if (!rpc_data_.empty()) {
    processRpcData();
  }

  switch (state_) {
    case improv::STATE_STOPPED:
      // TODO: Set indicator false
      setState(improv::STATE_AWAITING_AUTHORIZATION);
      setError(improv::ERROR_NONE);
      break;
    case improv::STATE_AWAITING_AUTHORIZATION:
      // TODO: Implement button auth
      setState(improv::STATE_AUTHORIZED);
      break;
    case improv::STATE_AUTHORIZED:
      // TODO: handle auth timeout
      break;
    case improv::STATE_PROVISIONING:
      // TODO: set indicator
      if (WiFi.isConnected()) {
        services_.getWifiNetwork()->wifi_aps_.push_back(wifi_ap_);
        services_.getStorage()->saveWiFiAP(wifi_ap_);
        wifi_ap_ = {};
        wifi_connect_start_ = std::chrono::steady_clock::time_point::min();
        setState(improv::STATE_PROVISIONED);
        sendProvisionedResponse();
      }
#ifdef GSM_NETWORK
      if (services_.getGsmNetwork()->gprs_connected_) {
        setState(improv::STATE_PROVISIONED);
        sendProvisionedResponse();
      }
#endif
      break;
    case improv::STATE_PROVISIONED:
      // Instance will be cleaned up after setting state to stopped
      setState(improv::STATE_STOPPED);
      break;
    default:
      break;
  }
}

void BleImprov::stop() {
  NimBLEDevice::getServer()->removeService(ble_improv_service_, true);
  ble_improv_service_ = nullptr;
}

void BleImprov::setState(improv::State state) {
  TRACEF("Setting state: %d\r\n", state);

  // Update the status BLE characteristic data
  if (state_ != state) {
    std::array<uint8_t, 1> data{state};
    ble_status_char_->setValue(data);
    if (state != improv::STATE_STOPPED) {
      ble_status_char_->notify();
      TRACELN("Notifying");
    }
  }

  // Advertise the service's state change
  std::string service_data{6, 0x00};
  service_data[0] = static_cast<uint8_t>(state);

  uint8_t capabilities = 0x00;
  capabilities |= improv::CAPABILITY_IDENTIFY;

  service_data[1] = capabilities;
  service_data[2] = 0x00;  // Reserved
  service_data[3] = 0x00;  // Reserved
  service_data[4] = 0x00;  // Reserved
  service_data[5] = 0x00;  // Reserved

  NimBLEAdvertising* ble_advertising =
      ble_improv_service_->getServer()->getAdvertising();
  ble_advertising->stop();
  ble_advertising->setName(Storage::device_type_name_);
  ble_advertising->setServiceData(ble_improv_service_->getUUID(), service_data);
  ble_advertising->enableScanResponse(true);
  ble_advertising->start();

  // Save new state
  state_ = state;
}

const improv::State BleImprov::getState() const { return state_; }

void BleImprov::setError(improv::Error error) {
  if (error != improv::ERROR_NONE) {
    TRACEF("Error: %d\r\n", error);
  }
  NimBLEAttValue ble_error_value = ble_error_char_->getValue();
  // Broadcast error if value was empty or has changed
  if (!ble_error_value.size() || ble_error_value.data()[0] != error) {
    std::array<uint8_t, 1> data{error};
    ble_error_char_->setValue(data.data(), data.size());
    if (state_ != improv::STATE_STOPPED) {
      ble_error_char_->notify();
    }
  }
}

void BleImprov::onWrite(NimBLECharacteristic* characteristic,
                        NimBLEConnInfo& connInfo) {
  if (characteristic == ble_rpc_command_char_) {
    NimBLEAttValue rpc_data = characteristic->getValue();
    if (!rpc_data.size()) {
      return;
    }
    rpc_data_.insert(rpc_data_.end(), rpc_data.begin(), rpc_data.end());
  } else {
    TRACELN("On write from unknown char");
  }
}

void BleImprov::setupService() {
  if (ble_improv_service_ != nullptr) {
    return;
  }

  ble_improv_service_ = services_.getBleServer()->createService(
      NimBLEUUID::fromString(improv::SERVICE_UUID));
  ble_status_char_ = ble_improv_service_->createCharacteristic(
      NimBLEUUID::fromString(improv::STATUS_UUID),
      NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY);
  ble_error_char_ = ble_improv_service_->createCharacteristic(
      NimBLEUUID::fromString(improv::ERROR_UUID),
      NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY);
  ble_rpc_command_char_ = ble_improv_service_->createCharacteristic(
      NimBLEUUID::fromString(improv::RPC_COMMAND_UUID), NIMBLE_PROPERTY::WRITE);
  ble_rpc_command_char_->setCallbacks(this);
  ble_rpc_response_char_ = ble_improv_service_->createCharacteristic(
      NimBLEUUID::fromString(improv::RPC_RESULT_UUID),
      NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY);
  ble_capabilities_char_ = ble_improv_service_->createCharacteristic(
      NimBLEUUID::fromString(improv::CAPABILITIES_UUID), NIMBLE_PROPERTY::READ);
  uint8_t capabilities = 0x00;
  capabilities |= improv::CAPABILITY_IDENTIFY;
  ble_capabilities_char_->setValue(capabilities);

  NimBLEAdvertising* ble_advertising = NimBLEDevice::getAdvertising();
  ble_advertising->setName(Storage::device_type_name_);
  ble_advertising->enableScanResponse(true);
  ble_advertising->addServiceUUID(ble_improv_service_->getUUID());
  ble_advertising->start();
}

void BleImprov::processRpcData() {
#ifdef ENABLE_TRACE
  TRACEF("BLE RPC Data: ", "");
  for (uint8_t i : rpc_data_) {
    Serial.print(i >> 4, HEX);
    Serial.print(i & 0x0F, HEX);
    Serial.print(" ");
  }
  Serial.println();
#endif
  // Length byte is second one (prevent OOB memory access)
  if (rpc_data_.size() < 2) {
    return;
  }
  uint8_t data_length = rpc_data_[1];
  uint8_t full_length = data_length + 3;
  // Wait for whole data frame (3 RPC control bytes)
  if (rpc_data_.size() < full_length) {
    return;
  }

  setError(improv::ERROR_NONE);
  improv::ImprovCommand command =
      improv::parse_improv_data(rpc_data_.data(), full_length);

  // Handle the command and then clear RPC data
  switch (command.command) {
    case improv::BAD_CHECKSUM:
      setError(improv::ERROR_INVALID_RPC);
      break;
    case improv::WIFI_SETTINGS: {
      if (state_ != improv::STATE_AUTHORIZED) {
        setError(improv::ERROR_NOT_AUTHORIZED);
        break;
      }
      if (command.ssid.length() == 0) {
        TRACELN("Empty WiFi SSID, assuming GSM connection");
        setState(improv::STATE_PROVISIONED);
        break;
      }
      wifi_ap_ = {command.ssid.c_str(), command.password.c_str(), -1, false};
      wl_status_t wl_status =
          WiFi.begin(wifi_ap_.ssid.c_str(), wifi_ap_.password.c_str());
      if (wl_status == WL_CONNECT_FAILED) {
        TRACELN("Failed starting WiFi setup");
        setError(improv::ERROR_UNABLE_TO_CONNECT);
        break;
      }
      TRACEF("Connecting to: %s - %s\r\n", command.ssid.c_str(),
             command.password.c_str());
      wifi_connect_start_ = std::chrono::steady_clock::now();
      setState(improv::STATE_PROVISIONING);
    } break;
    case improv::IDENTIFY: {
      std::vector<uint8_t> data =
          improv::build_rpc_response(improv::IDENTIFY, std::vector<String>());
      ble_rpc_response_char_->setValue(data);
      ble_rpc_response_char_->notify();
      Services::getActionController().identify();
    } break;
    case improv::GET_DEVICE_INFO:
      sendDeviceInfoResponse();
      break;
    case improv::X_GET_DEVICE_TYPE:
      sendDeviceTypeResponse();
      break;
    case improv::GET_WIFI_NETWORKS:
      startGetWifiNetworks();
      break;
    case improv::X_SET_SERVER_AUTH: {
      if (state_ != improv::STATE_AUTHORIZED) {
        setError(improv::ERROR_NOT_AUTHORIZED);
        break;
      }
      setServerAuth(command);
    } break;
    case improv::X_SET_USER_DATA: {
      setUserData(command);
      break;
    }
    case improv::X_START_PROVISIONING: {
      setState(improv::STATE_PROVISIONING);
      Services::getActionController().identify();
      break;
    }
#ifdef GSM_NETWORK
    case improv::X_GET_MOBILE_STATE: {
      sendMobileStateResponse();
      break;
    }
    case improv::X_GET_MOBILE_OPERATORS: {
      startGetMobileNetworks(command);
      break;
    }
    case improv::X_SET_ALLOWED_MOBILE_OPERATORS: {
      setAllowedMobileOperators(command);
      break;
    }
#endif
    default:
      setError(improv::ERROR_UNKNOWN_RPC);
      break;
  }

  if (rpc_data_.size() > full_length) {
    rpc_data_.erase(rpc_data_.begin(), rpc_data_.begin() + full_length);
  } else {
    rpc_data_.clear();
  }
}

void BleImprov::setServerAuth(const improv::ImprovCommand& command) {
  std::shared_ptr<Storage> storage = services_.getStorage();
  std::shared_ptr<WebSocket> web_socket = services_.getWebSocket();
  TRACELN("Clearing local resources");
  storage->deletePeripherals();

  if (command.ssid.size() != 0) {
    struct yuarel url;
    // Copy URL to non-const char array as yuarel_parse sets NULL in src str
    std::vector<char> url_data(command.ssid.size() + 1, '\0');
    strlcpy(url_data.data(), command.ssid.data(), url_data.size());
    if (yuarel_parse(&url, &url_data[0]) == -1) {
      TRACELN("Invalid URL");
      setError(improv::ERROR_INVALID_RPC);
      return;
    } else {
      bool secure_url = true;
      if (strcmp(url.scheme, "ws") == 0) {
        secure_url = false;
      }
      String path("/");
      path += url.path;
      TRACEF("Setting URL: %s, %s, %d, %s, %d\r\n", url.scheme, url.host,
             url.port, path.c_str(), secure_url);
      web_socket->setUrl(url.host, path.c_str(), secure_url);
      storage->saveWsUrl(url.host, path.c_str(), secure_url);
    }
  } else {
    TRACELN("Clearing URL");
    web_socket->resetUrl();
    storage->deleteWsUrl();
  }

  web_socket->setWsToken(command.password.c_str());
  storage->saveAuthToken(command.password.c_str());

  std::vector<uint8_t> rpc_response = improv::build_rpc_response(
      improv::X_SET_SERVER_AUTH, std::vector<String>());
  ble_rpc_response_char_->setValue(rpc_response);
  ble_rpc_response_char_->notify();
}

void BleImprov::handleWiFiConnectTimeout() {
  // Return if connect timeout not active
  if (wifi_connect_start_ == std::chrono::steady_clock::time_point::min()) {
    return;
  }
  // Return if timeout has not timed out
  std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
  if (now < wifi_connect_start_ + wifi_connect_timeout_) {
    return;
  }
  // Publish error, revert state and reset connect timeout
  setError(improv::ERROR_UNABLE_TO_CONNECT);
  setState(improv::STATE_AUTHORIZED);
  wifi_connect_start_ = std::chrono::steady_clock::time_point::min();
}

void BleImprov::sendProvisionedResponse() {
  if (state_ == improv::STATE_STOPPED) {
    return;
  }
  std::vector<String> urls{services_.getWebSocket()->core_domain_};
  std::vector<uint8_t> data =
      improv::build_rpc_response(improv::WIFI_SETTINGS, urls);
  ble_rpc_response_char_->setValue(data);
  ble_rpc_response_char_->notify();
}

void BleImprov::sendDeviceInfoResponse() {
  if (state_ == improv::STATE_STOPPED) {
    return;
  }
  std::vector<String> device_info;
  // Set the firmware name and version, if available
  {
    String fw_name_version{FIRMWARE_VERSION};
    int delim_idx = fw_name_version.indexOf("@");
    if (delim_idx == -1) {
      device_info.emplace_back(fw_name_version);
      device_info.emplace_back("");
    } else {
      device_info.emplace_back(fw_name_version.substring(0, delim_idx));
      device_info.emplace_back(fw_name_version.substring(delim_idx + 1));
    }
  }

  // Set the hardware type and controller name
  String board_name(Storage::arduino_board_);
  board_name += "@";
  board_name += Network.macAddress();

  device_info.emplace_back(board_name);
  device_info.emplace_back(Storage::device_type_name_);
  device_info.emplace_back(services_.getWifiNetwork()->controller_name_);
#ifdef DEVICE_TYPE_FIRE_DATA_LOGGER
  const char* net_state =
      services_.getGsmNetwork()->isEnabled() ? "net:gsm" : "net:wifi";
  device_info.emplace_back(net_state);
#endif
  for (const String& str : device_info) {
    TRACELN(str);
  }

  std::vector<uint8_t> data =
      improv::build_rpc_response(improv::GET_DEVICE_INFO, device_info);
  ble_rpc_response_char_->setValue(data);
  ble_rpc_response_char_->notify();
  TRACELN("Sent device info");
}

void BleImprov::sendDeviceTypeResponse() {
  if (state_ == improv::STATE_STOPPED) {
    return;
  }

  std::vector<String> device_type{Storage::device_type_id_};
  std::vector<uint8_t> data =
      improv::build_rpc_response(improv::X_GET_DEVICE_TYPE, device_type);
  ble_rpc_response_char_->setValue(data);
  ble_rpc_response_char_->notify();
  TRACEF("Sent device type: %s\r\n", device_type[0].c_str());
}

void BleImprov::startGetWifiNetworks() {
  if (scan_wifi_aps_) {
    setError(improv::Error::X_ERROR_ALREADY_SCANNING);
    return;
  }
  scan_wifi_aps_ = true;
  WiFi.scanDelete();
  WiFi.disconnect();
  WiFi.scanNetworks(true);
}

void BleImprov::handleGetWifiNetworks() {
  if (state_ == improv::STATE_STOPPED || !scan_wifi_aps_) {
    return;
  }
  int16_t wifi_scan_state = WiFi.scanComplete();
  // -1 still scanning, -2 or less are error / not started
  if (wifi_scan_state < 0) {
    if (wifi_scan_state < -1) {
      scan_wifi_aps_ = false;
    }
    return;
  }

  // Order and remove duplicates of WiFi APs
  std::vector<WiFiScanAP> scanned_wifi_aps;
  for (int16_t i = 0; i < wifi_scan_state; i++) {
    NetworkInfo network_info{.id = i};
    WiFiNetwork::populateNetworkInfo(network_info);
    // Check if network was already found and update signal strength (RSSI)
    bool found = false;
    for (auto& wifi_ap : scanned_wifi_aps) {
      if (wifi_ap.ssid == network_info.ssid) {
        if (network_info.rssi > wifi_ap.rssi) {
          wifi_ap.rssi = network_info.rssi;
        }
        found = true;
        break;
      }
    }
    if (!found) {
      scanned_wifi_aps.emplace_back(network_info.ssid, network_info.rssi,
                                    network_info.encType != WIFI_AUTH_OPEN);
    }
  }
  // Sort WiFi networks by RSSI (strongest to weakest)
  std::sort(scanned_wifi_aps.begin(), scanned_wifi_aps.end(),
            [](const BleImprov::WiFiScanAP& a, const BleImprov::WiFiScanAP& b) {
              return a.rssi > b.rssi;
            });

  // Send found WiFi APs as BLE RPC response
  for (const WiFiScanAP& ap : scanned_wifi_aps) {
    std::vector<String> wifi_info;
    wifi_info.emplace_back(ap.ssid);
    wifi_info.emplace_back(ap.rssi);
    wifi_info.emplace_back(ap.auth_required ? "YES" : "NO");
    std::vector<uint8_t> data =
        improv::build_rpc_response(improv::GET_WIFI_NETWORKS, wifi_info);
    ble_rpc_response_char_->setValue(data);
    ble_rpc_response_char_->notify();
    data[data.size() - 1] = '\0';
    Serial.println((char*)data.data());
  }

  // Send RPC response without WiFi AP, reset state and delete scan details
  std::vector<uint8_t> data = improv::build_rpc_response(
      improv::GET_WIFI_NETWORKS, std::vector<String>());
  ble_rpc_response_char_->setValue(data);
  ble_rpc_response_char_->notify();
  scan_wifi_aps_ = false;
  WiFi.scanDelete();
}

void BleImprov::setUserData(const improv::ImprovCommand& command) {
  user_data_ += command.ssid.c_str();
  // Accumulate all message parts before parsing and handling JSON
  if (command.ssid.length() >= 200) {
    return;
  }

  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, user_data_);
  if (error) {
    TRACEF("Failed parsing user data: %s\r\n", error.c_str());
    setError(improv::Error::ERROR_UNKNOWN);
    user_data_ = "";
    return;
  }
  TRACEF("User data: %d, %d\r\n", user_data_.length(),
         services_.getBleServer()->user_data_handlers_.size());
  for (const auto& handler : services_.getBleServer()->user_data_handlers_) {
    const bool success = handler(doc.as<JsonObject>());
    if (!success) {
      setError(improv::Error::ERROR_UNKNOWN);
      user_data_ = "";
      return;
    }
  }
  user_data_ = "";

  std::vector<uint8_t> rpc_response = improv::build_rpc_response(
      improv::X_SET_USER_DATA, std::vector<String>());
  ble_rpc_response_char_->setValue(rpc_response);
  ble_rpc_response_char_->notify();
}

#ifdef GSM_NETWORK
void BleImprov::sendMobileStateResponse() {
  if (state_ == improv::STATE_STOPPED) {
    return;
  }
  const auto gsm_network = services_.getGsmNetwork();
  if (gsm_network->cops_scan_ && gsm_network->cops_scan_->active) {
    setError(improv::Error::X_ERROR_ALREADY_SCANNING);
    return;
  }

  char connection_state = gsm_network->modem_.isNetworkConnected() |
                          gsm_network->modem_.isGprsConnected() << 1;
  bool auto_nsm;  // Ignored value
  int16_t nsm = 0;
  gsm_network->modem_.getNetworkSystemMode(auto_nsm, nsm);
  int16_t signal_quality = gsm_network->modem_.getSignalQuality();

  std::vector<String> mobile_state = {
      gsm_network->modem_.getSimCCID(), gsm_network->modem_.getIMEI(),
      String(connection_state),         gsm_network->modem_.getOperator(),
      String(signal_quality),           String(nsm)};
  std::vector<uint8_t> data =
      improv::build_rpc_response(improv::X_GET_MOBILE_STATE, mobile_state);
  ble_rpc_response_char_->setValue(data);
  ble_rpc_response_char_->notify();
  TRACEF("Sent ICCID: %s IMEI: %s Con: %d:%d Oper: %s CSQ: %d NSM: %d\r\n",
         mobile_state[0].c_str(), mobile_state[1].c_str(), connection_state & 1,
         (connection_state >> 1) & 1, mobile_state[3].c_str(), signal_quality,
         nsm);
}

void BleImprov::startGetMobileNetworks(const improv::ImprovCommand& command) {
  const auto gsm_network = services_.getGsmNetwork();
  if (gsm_network->cops_scan_ && gsm_network->cops_scan_->active) {
    setError(improv::Error::X_ERROR_ALREADY_SCANNING);
    return;
  }

  GsmNetwork::CopsScanType scan_type = GsmNetwork::CopsScanType::kDefault;
  if (command.ssid.length()) {
    if (command.ssid.starts_with("auto")) {
      scan_type = GsmNetwork::CopsScanType::kAuto;
    } else if (command.ssid.starts_with("gsm")) {
      scan_type = GsmNetwork::CopsScanType::kGsm;
    } else if (command.ssid.starts_with("lte")) {
      scan_type = GsmNetwork::CopsScanType::kLte;
    }
  }
  TRACELN("Starting mobile scan");
  gsm_network->startCopsScan(scan_type);
  scan_mobile_operators_ = true;
}

void BleImprov::handleGetMobileOperators() {
  if (state_ == improv::STATE_STOPPED || !scan_mobile_operators_) {
    return;
  }

  const auto gsm_network = services_.getGsmNetwork();

  gsm_network->pollCopsScan();
  if (!gsm_network->cops_scan_) {
    TRACELN("COPS Scan nullptr");
    scan_mobile_operators_ = false;
    return;
  }

  // Wait for COPS scan to finish, error or timeout
  if (gsm_network->cops_scan_->active) {
    return;
  }

  // On failure, send the error via BLE and clear COPS scan
  if (!gsm_network->cops_scan_->success) {
    setError(improv::ERROR_UNKNOWN);
    std::vector<uint8_t> data = improv::build_rpc_response(
        improv::X_GET_MOBILE_OPERATORS,
        std::vector<String>({gsm_network->cops_result_}));
    ble_rpc_response_char_->setValue(data);
    ble_rpc_response_char_->notify();
    scan_mobile_operators_ = false;
    gsm_network->clearCopsScan();
    return;
  }

  // Send one BLE response for each top-level tuple in COPS result.
  // Example tuple payload: 2,"CHN-UNICOM","UNICOM","46001",7
  const String& cops_result = gsm_network->cops_result_;
  int8_t tuple_depth = 0;
  int32_t tuple_start = -1;

  for (uint16_t i = 0; i < cops_result.length(); i++) {
    const char c = cops_result[i];

    if (c == '(') {
      if (tuple_depth == 0) {
        // Start payload right after opening parenthesis.
        tuple_start = static_cast<int32_t>(i) + 1;
      }
      tuple_depth++;
      continue;
    }

    if (c == ')' && tuple_depth > 0) {
      tuple_depth--;
      if (tuple_depth == 0 && tuple_start >= 0) {
        TRACEF("Operator: %s\r\n",
               cops_result.substring(tuple_start, i).c_str());
        std::vector<uint8_t> data = improv::build_rpc_response(
            improv::X_GET_MOBILE_OPERATORS,
            std::vector<String>({cops_result.substring(tuple_start, i)}));
        ble_rpc_response_char_->setValue(data);
        ble_rpc_response_char_->notify();
        tuple_start = -1;
      }
      continue;
    }
  }

  // Send an empty response to signal completion.
  std::vector<uint8_t> data = improv::build_rpc_response(
      improv::X_GET_MOBILE_OPERATORS, std::vector<String>());
  ble_rpc_response_char_->setValue(data);
  ble_rpc_response_char_->notify();

  scan_mobile_operators_ = false;
  gsm_network->clearCopsScan();
  TRACELN(F("Sent mobile networks"));
}

void BleImprov::setAllowedMobileOperators(
    const improv::ImprovCommand& command) {
  std::vector<String> operators;
  String current_operator;
  // Empty payload clears the allowlist.
  // Otherwise parse comma-separated MCC/MNC codes from the SSID field.
  if (command.ssid.length() != 0) {
    // Set an error on invalid payload.
    for (uint i = 0; i <= command.ssid.length(); i++) {
      // Add synthetic comma at end to handle last item
      const char c = (i == command.ssid.length()) ? ',' : command.ssid[i];

      if (c == ',') {
        if (current_operator.length() < 5 || current_operator.length() > 6) {
          setError(improv::ERROR_INVALID_RPC);
          return;
        }
        operators.emplace_back(current_operator);
        current_operator = "";
        continue;
      }

      if (c < '0' || c > '9') {
        setError(improv::ERROR_INVALID_RPC);
        return;
      }

      current_operator += c;

      if (current_operator.length() > 6) {
        setError(improv::ERROR_INVALID_RPC);
        return;
      }
    }
  }

  // Set and replace parsed MCC/MNC codes to storage
  ErrorResult result =
      services_.getGsmNetwork()->setAllowedMobileOperators(operators);
  if (result.isError()) {
    TRACEF("Failed saving allowed MNOs: %s\r\n", result.toString().c_str());
    setError(improv::ERROR_UNKNOWN);
    return;
  }

  std::vector<uint8_t> rpc_response = improv::build_rpc_response(
      improv::X_SET_ALLOWED_MOBILE_OPERATORS, std::vector<String>());
  ble_rpc_response_char_->setValue(rpc_response);
  ble_rpc_response_char_->notify();

  setState(improv::STATE_PROVISIONING);
  Services::getActionController().identify();
}
#endif

}  // namespace inamata