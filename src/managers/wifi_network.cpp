#include "wifi_network.h"

#include "esp_sntp.h"

#ifdef RTC_MANAGER
#include "managers/time_manager.h"
#endif

namespace inamata {

bool is_time_synced = false;

void timeSyncCallback(struct timeval* tv) {
  is_time_synced = true;

#ifdef TRACELN
  time_t nowSecs = time(nullptr);
  struct tm timeinfo;
  gmtime_r(&nowSecs, &timeinfo);
  TRACEF("Time synced: %s\r\n", asctime(&timeinfo));
#endif
#ifdef RTC_MANAGER
  if (TimeManager::lostPower()) {
    TimeManager::setSystemTime(DateTime(nowSecs));
    TRACELN("WiFi updated RTC clock");
  }
#endif
}

WiFiNetwork::WiFiNetwork(std::vector<WiFiAP>& wifi_aps, String& controller_name)
    : wifi_aps_(std::move(wifi_aps)),
      controller_name_(std::move(controller_name)) {
#ifdef ENABLE_TRACE
  TRACELN(F("Searching for APs:"));
  for (const WiFiAP& wifi_ap : wifi_aps_) {
    TRACEF("\t%s\r\n", wifi_ap.ssid.c_str());
  }
#endif
}

void WiFiNetwork::setMode(ConnectMode mode) { connect_mode_ = mode; }

WiFiNetwork::ConnectMode WiFiNetwork::getMode() { return connect_mode_; }

WiFiNetwork::ConnectMode WiFiNetwork::connect() {
  // If not connected, execute active conntect mode. After trying action, try
  // check if subsequent action can be run
  bool connected = isConnected();
  if (connected) {
    return ConnectMode::kConnected;
  }
  if (connect_mode_ == ConnectMode::kFastConnect) {
    connected = tryFastConnect();
  }
  if (connect_mode_ == ConnectMode::kScanning) {
    connected = tryScanning();
  }
  if (connect_mode_ == ConnectMode::kMultiConnect) {
    connected = tryMultiConnect();
  }
  if (connect_mode_ == ConnectMode::kHiddenConnect) {
    connected = tryHiddenConnect();
  }
  if (connect_mode_ == ConnectMode::kCyclePower) {
    connected = tryCyclePower();
  }
  if (connect_mode_ == ConnectMode::kPowerOff) {
    wifi_mode_t wifi_mode = WiFi.getMode();
    if (wifi_mode != WIFI_OFF || wifi_mode != WIFI_MODE_NULL) {
      TRACELN("Turning WiFi off");
      WiFi.mode(WIFI_OFF);
    }
    connected = false;
  }
  return connected ? ConnectMode::kConnected : connect_mode_;
}

bool WiFiNetwork::isConnected(wl_status_t* wifi_status) {
  // If wifi_status is passed, use that state, else check it now
  wl_status_t status;
  if (wifi_status == nullptr) {
    status = WiFi.status();
  } else {
    status = *wifi_status;
  }

  // If connected, set connect mode to fast connect for reconnect
  if (status == WL_CONNECTED) {
    connect_mode_ = ConnectMode::kFastConnect;
#ifdef ENABLE_TRACE
    if (connect_start_ != std::chrono::steady_clock::time_point::min()) {
      Serial.printf("Network: Connected to %s, IP: %s\r\n", WiFi.SSID().c_str(),
                    WiFi.localIP().toString().c_str());
    }
#endif
    connect_start_ = std::chrono::steady_clock::time_point::min();
    return true;
  }
  return false;
}

bool WiFiNetwork::tryFastConnect() {
  wl_status_t wifi_status = WL_IDLE_STATUS;
  if (connect_start_ == std::chrono::steady_clock::time_point::min()) {
    // If first run of fast connect
    Serial.println(F("WiFi: FastConnect start"));
    connect_start_ = std::chrono::steady_clock::now();
    if (strlen(WiFi.SSID().c_str())) {
      TRACEF("FastConnect: %s\r\n", WiFi.SSID().c_str());
      // Connect to previous saved WiFi, if one exists,
      wifi_status = WiFi.begin();
    } else {
      // else start WiFi search and exit fast connect mode
      TRACELN(F("FastConnect: No saved SSID"));
      connect_start_ = std::chrono::steady_clock::time_point::min();
      connect_mode_ = ConnectMode::kScanning;
      return false;
    }
  } else {
    // Subsequent attempt to fast connect, update connection status
    wifi_status = WiFi.status();
  }
  TRACEF("FastConnect: wifi_status: %d\r\n", wifi_status);
  // Return connected, reset fast connect, but stay in fast connect mode
  if (isConnected(&wifi_status)) {
    return true;
  }
  // If connection to AP failed or timeout passed, start WiFi search
  std::chrono::steady_clock::duration connect_duration =
      std::chrono::steady_clock::now() - connect_start_;
  // Ignore 'wifi_status == WL_CONNECT_FAILED' as when already connecting, it
  // will return that error
  if (connect_duration > connect_timeout_) {
    TRACEF(
        "Failed connecting to %s after %lldms\r\n", WiFi.SSID().c_str(),
        std::chrono::duration_cast<std::chrono::milliseconds>(connect_duration)
            .count());
    connect_start_ = std::chrono::steady_clock::time_point::min();
    connect_mode_ = ConnectMode::kScanning;
  }
  // Stay in fast connect mode, but return not connected yet
  return false;
}

bool WiFiNetwork::tryScanning() {
  if (scan_start_ == std::chrono::steady_clock::time_point::min()) {
    startWiFiScan();
  }
  int8_t scan_result = WiFi.scanComplete();
  std::chrono::steady_clock::duration scan_duration =
      std::chrono::steady_clock::now() - scan_start_;
  if (scan_result > 0) {
    TRACEF("Scan found %d APs after %lldms\r\n", scan_result,
           std::chrono::duration_cast<std::chrono::milliseconds>(scan_duration)
               .count());
    // If scan finished and found networks
    // Clear internal IDs of WiFi network
    for (auto& wifi_ap : wifi_aps_) {
      wifi_ap.id = -1;
      wifi_ap.failed_connecting = false;
    }
    // Set IDs for found and known WiFi networks
    // For each found AP, get its details and
    for (int8_t i = 0; i < scan_result; i++) {
      NetworkInfo network_info{.id = i};
      populateNetworkInfo(network_info);
      // ... check against known APs and
      for (auto& wifi_ap : wifi_aps_) {
        // ... if it matches one
        if (network_info.ssid == wifi_ap.ssid) {
          if (wifi_ap.id == -1) {
            // ... if it is the first AP for a given SSID
            TRACEF("Found AP %s, RSSI: %d, ID: %d\r\n", wifi_ap.ssid.c_str(),
                   WiFi.RSSI(i), i);
            wifi_ap.id = i;
          } else if (WiFi.RSSI(i) > WiFi.RSSI(wifi_ap.id)) {
            // ... set the ID of the one with the strongest signal
            TRACEF("Update AP %s, RSSI: %d, ID: %d\r\n", wifi_ap.ssid.c_str(),
                   WiFi.RSSI(i), i);
            wifi_ap.id = i;
          } else {
            TRACEF("Weaker AP %s ignored, RSSI: %d, ID: %d\r\n",
                   wifi_ap.ssid.c_str(), WiFi.RSSI(i), i);
          }
          break;
        }
      }
    }
    // Sort WiFi networks by RSSI and if they were found
    std::sort(wifi_aps_.begin(), wifi_aps_.end(), sortRssi);
    // Change to multi connect and set the first network to be tried
    connect_mode_ = ConnectMode::kMultiConnect;
    current_wifi_ap_ = wifi_aps_.begin();
    scan_start_ = std::chrono::steady_clock::time_point::min();
  } else if (scan_result == 0 || scan_duration > scan_timeout_) {
    // If the scan finished but didn't find any networks or it timed out,
    // try to connect to hidden networks
    TRACEF("No APs found after %lldms\r\n",
           std::chrono::duration_cast<std::chrono::milliseconds>(scan_duration)
               .count());
    connect_mode_ = ConnectMode::kHiddenConnect;
    current_wifi_ap_ = wifi_aps_.begin();
    scan_start_ = std::chrono::steady_clock::time_point::min();
  }
  // Scan never results in connection to AP
  return false;
}

void WiFiNetwork::startWiFiScan() {
  TRACELN(F("WiFi: Scan start"));
  // If first run of WiFi scan
  scan_start_ = std::chrono::steady_clock::now();
  // Clean previous scan
  WiFi.scanDelete();
  // Remove previous WiFi SSID/password
  WiFi.disconnect();
  // Start wifi scan in async mode
  WiFi.scanNetworks(true);
}

int16_t WiFiNetwork::getWiFiScanState() {
  int16_t scan_result = WiFi.scanComplete();
  std::chrono::steady_clock::duration scan_duration =
      std::chrono::steady_clock::now() - scan_start_;
  if (scan_duration > scan_timeout_) {
    TRACELN(F("WiFi: Scan timed out"));
    return -3;
  }
  return scan_result;
}

bool WiFiNetwork::tryMultiConnect() {
  if (current_wifi_ap_ == wifi_aps_.end() || current_wifi_ap_->id == -1) {
    // On reaching the last or last known AP, try connecting to hidden APs
    connect_mode_ = ConnectMode::kHiddenConnect;
    // Try APs from begining
    current_wifi_ap_ = wifi_aps_.begin();
    WiFi.scanDelete();
    return false;
  }

  wl_status_t wifi_status = WL_IDLE_STATUS;
  if (connect_start_ == std::chrono::steady_clock::time_point::min()) {
    Serial.println(F("WiFi: MultiConnect start"));
    // If the first run of connecting to a network
    connect_start_ = std::chrono::steady_clock::now();
    // Get all the details of the network to connect to
    NetworkInfo network_info{.id = current_wifi_ap_->id};
    populateNetworkInfo(network_info);

    // If the SSID matches, try connecting, else skip it
    if (current_wifi_ap_->ssid == network_info.ssid) {
      TRACEF("Connecting to %s\r\n", network_info.ssid.c_str());
      wifi_status = WiFi.begin(network_info.ssid.c_str(),
                               current_wifi_ap_->password.c_str(),
                               network_info.channel, network_info.bssid);
    } else {
      current_wifi_ap_++;
      connect_start_ = std::chrono::steady_clock::time_point::min();
      return false;
    }
  } else {
    wifi_status = WiFi.status();
  }

  if (isConnected(&wifi_status)) {
    WiFi.scanDelete();
    return true;
  }
  std::chrono::steady_clock::duration connect_duration =
      std::chrono::steady_clock::now() - connect_start_;
  if (wifi_status == WL_CONNECT_FAILED || connect_duration > connect_timeout_) {
    // On connection failure, mark AP as failed, reset connection timer, try
    // next AP
    TRACEF("Failed connecting to %s after %lldms\r\n",
           current_wifi_ap_->ssid.c_str(),
           std::chrono::duration_cast<std::chrono::seconds>(connect_duration)
               .count());
    current_wifi_ap_->failed_connecting = true;
    connect_start_ = std::chrono::steady_clock::time_point::min();
    current_wifi_ap_++;
    return false;
  }
  // Stay in multi-connect mode, but return still trying to connect
  return false;
}

bool WiFiNetwork::tryHiddenConnect() {
  if (current_wifi_ap_ == wifi_aps_.end()) {
    // On reaching the last AP, cycle modem power before returning to scan
    // mode
    connect_mode_ = ConnectMode::kPowerOff;
    return false;
  }

  wl_status_t wifi_status = WL_IDLE_STATUS;
  if (connect_start_ == std::chrono::steady_clock::time_point::min()) {
    Serial.println(F("WiFi: HiddenConnect start"));
    // If the first run of connecting to a hidden network
    connect_start_ = std::chrono::steady_clock::now();
    while (current_wifi_ap_ != wifi_aps_.end()) {
      if (current_wifi_ap_->failed_connecting) {
        current_wifi_ap_++;
      } else {
        break;
      }
    }
    if (current_wifi_ap_ == wifi_aps_.end()) {
      return false;
    }
    TRACEF("Connecting to %s\r\n", current_wifi_ap_->ssid.c_str());
    // Try to connect to the next elligible AP
    wifi_status = WiFi.begin(current_wifi_ap_->ssid.c_str(),
                             current_wifi_ap_->password.c_str());
  } else {
    wifi_status = WiFi.status();
  }

  if (isConnected(&wifi_status)) {
    // Connected to AP, so set fast connect mode if connection drops
    return true;
  }
  std::chrono::steady_clock::duration connect_duration =
      std::chrono::steady_clock::now() - connect_start_;
  if (wifi_status == WL_CONNECT_FAILED || connect_duration > connect_timeout_) {
    // On failure, reset connection timer, try next AP
    TRACEF(
        "Failed connecting to %s after %lldms\r\n",
        current_wifi_ap_->ssid.c_str(),
        std::chrono::duration_cast<std::chrono::milliseconds>(connect_duration)
            .count());
    connect_start_ = std::chrono::steady_clock::time_point::min();
    current_wifi_ap_++;
    return false;
  }
  // Stay in hidden-connect mode, but return still trying to connect
  return false;
}

bool WiFiNetwork::tryCyclePower() {
  const auto mode = WiFi.getMode();
  // If WiFi modem is not powered off, turn it off
  TRACEF("Changing from WiFi mode: %d\r\n", mode);
  if (mode != WIFI_OFF) {
    Serial.println(F("WiFi: CyclePower start"));
    WiFi.mode(WIFI_OFF);
  } else {
    // In the next cycle, turn it back on and try to fast connect
    WiFi.mode(WIFI_MODE_STA);
    connect_mode_ = ConnectMode::kFastConnect;
  }
  // Power cycle never results in connection to AP
  return false;
}

void WiFiNetwork::initTimeSync() {
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");
  sntp_set_time_sync_notification_cb(timeSyncCallback);
  TRACELN(F("Start time sync"));
}

bool WiFiNetwork::isTimeSynced() { return is_time_synced; }

bool WiFiNetwork::populateNetworkInfo(NetworkInfo& network_info) {
  if (network_info.id < 0 || network_info.id > 255) {
    return false;
  }
  uint8_t id = static_cast<uint8_t>(network_info.id);
  return WiFi.getNetworkInfo(id, network_info.ssid, network_info.encType,
                             network_info.rssi, network_info.bssid,
                             network_info.channel);
}

bool WiFiNetwork::sortRssi(const WiFiAP& lhs, const WiFiAP& rhs) {
  if (lhs.id == -1) {
    return false;
  }
  if (rhs.id == -1) {
    return true;
  }
  return WiFi.RSSI(lhs.id) > WiFi.RSSI(rhs.id);
}

}  // namespace inamata
