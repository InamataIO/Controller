#pragma once

#include <WiFi.h>

#ifdef PROV_WIFI
#include <WiFiManager.h>
#endif

#include <chrono>
#include <vector>

#include "managers/logging.h"
#include "managers/types.h"

namespace inamata {

struct NetworkInfo {
  int16_t id;
  String ssid;
  int32_t rssi;
  uint8_t encType;
  uint8_t* bssid;
  int32_t channel;
  bool hidden;
};

/**
 * Wifi related functionality
 *
 * Functionality includes conencting to wifi
 */
class WiFiNetwork {
 public:
  enum class ConnectMode {
    /// Connected to WiFi AP
    kConnected,
    /// Try connecting using cached WiFi credentials
    kFastConnect,
    /// Scan for WiFi networks
    kScanning,
    /// Try to connect to any visible and known WiFi networks
    kMultiConnect,
    /// Try to connect to hidden and known WiFi networks
    kHiddenConnect,
    /// Cycle the WiFi modem's power to reset state
    kCyclePower,
    /// Power off the wifi
    kPowerOff
  };

  /**
   * WiFi helper class that deals with connection time-outs and checking its
   * state
   *
   * \param acces_points The WiFi access points to try to connect to
   */
  WiFiNetwork(std::vector<WiFiAP>& access_points, String& controller_name);

  void setMode(ConnectMode mode);
  ConnectMode getMode();

  /**
   * Connects to the configured WiFi access point
   *
   * Expects to be called periodically to handle the WiFi connection procedure.
   * Does not block, and will cycle through, AP search, connect to known and
   * then unknown APs, before power cycling the modem.
   *
   * \return True if connected
   */
  ConnectMode connect();

  /**
   * Checks if connected to a WiFi network
   *
   * \param wifi_status Use output of WiFi.status() (Optional)
   * \return True if connected
   */
  bool isConnected(wl_status_t* wifi_status = nullptr);

  /**
   * Start the NTP service to sync clock every hour
   */
  void initTimeSync();

  /**
   * Whether the system clock has been synced since boot
   */
  bool isTimeSynced();

  static bool populateNetworkInfo(NetworkInfo& network_info);

  /**
   * Sorts WiFi networks by descending signal strength (unknown to back)
   *
   * @return true if rhs has weaker signal or unknown internal ID
   * @return false if lhs has weaker signal or unknown internal ID
   */
  static bool sortRssi(const WiFiAP& lhs, const WiFiAP& rhs);

  /**
   * Start a WiFi AP scan and set timeout
   */
  void startWiFiScan();

  /**
   * Return the current WiFi scan state or timeout error
   *
   * \return >= 0 network count, -1 scanning, -2 not running, -3 timed out
   */
  int16_t getWiFiScanState();

  std::vector<WiFiAP> wifi_aps_;
  String controller_name_;

 private:
  /**
   * Perform WiFi fast connect attempt (non-blocking)
   *
   * @return True if connected
   */
  bool tryFastConnect();

  /**
   * Perform WiFi AP scan attempt (non-blocking)
   *
   * @return Always false
   */
  bool tryScanning();

  /**
   * Perform WiFi connection attempt (non-blocking)
   *
   * @return True if connected
   */
  bool tryMultiConnect();

  /**
   * Perform WiFi connection attempt to hidden AP (non-blocking)
   *
   * @return True if connected
   */
  bool tryHiddenConnect();

  /**
   * Perform WiFi modem power cycling (non-blocking)
   *
   * @return Always false
   */
  bool tryCyclePower();

  std::vector<WiFiAP>::iterator current_wifi_ap_;

  ConnectMode connect_mode_ = ConnectMode::kFastConnect;
  std::chrono::steady_clock::time_point connect_start_ =
      std::chrono::steady_clock::time_point::min();
  std::chrono::milliseconds connect_timeout_ = std::chrono::seconds(20);

  std::chrono::steady_clock::time_point scan_start_ =
      std::chrono::steady_clock::time_point::min();
  std::chrono::milliseconds scan_timeout_ = std::chrono::seconds(30);

  bool multi_connect_first_run_ = true;
};

}  // namespace inamata
