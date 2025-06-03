#pragma once

#include <TaskSchedulerDeclarations.h>
#ifdef PROV_WIFI
#include <WiFiManager.h>
#endif

#include <chrono>
#include <limits>

#include "managers/ble_improv.h"
#include "managers/service_getters.h"
#include "managers/web_socket.h"
#include "tasks/base_task.h"
#include "utils/chrono_abs.h"
#ifdef DEVICE_TYPE_FIRE_DATA_LOGGER
#include "peripheral/peripherals/pca9536d/pca9536d.h"
#endif

namespace inamata {
namespace tasks {
namespace connectivity {

class CheckConnectivity : public BaseTask {
 public:
#ifdef DEVICE_TYPE_FIRE_DATA_LOGGER
  using PCA9536D = peripheral::peripherals::pca9536d::PCA9536D;
#endif

  enum class Mode {
    ConnectWiFi,
    ConnectGsm,
    ProvisionDevice,
  };

  enum class UseNetwork {
    kGsm,
    kWifi,
    kNone,
  };

  CheckConnectivity(const ServiceGetters& services, Scheduler& scheduler);
  ~CheckConnectivity() = default;

  const String& getType() const final;
  static const String& type();

 private:
  bool OnTaskEnable() final;
  bool TaskCallback() final;

  /**
   * Change connectivity mode
   *
   * @param mode connect to server or run captive portal
   */
  void setMode(Mode mode);

  /**
   * Reinits time sync (NTP) service every day
   */
  void handleClockSync(const std::chrono::steady_clock::time_point now);

  /**
   * Checks if the time has been synced since power on
   *
   * @return True if synced either by WiFi or GSM network
   */
  bool isTimeSynced();

  /**
   * Performs WebSocket processing and ensure connected state
   *
   * If WebSocket connection fails after a timeout, open a captive portal
   */
  void handleWebSocket();

  bool initGsmWifiSwitch();

  /**
   * Check whether GSM or WiFi connection should be used
   *
   * @param force Force enter connect mode
   */
  void handleGsmWifiSwitch(const std::chrono::steady_clock::time_point now,
                           bool force = false);

  /**
   * Connect to either GSM/LTE or WiFi
   */
  void enterConnectMode();

#ifdef PROV_IMPROV
  enum class WiFiScanMode { kNone, kScanning, kFinished };

  void handleBleServer();
  void handleImprov();

  std::unique_ptr<BleImprov> improv_;
#endif

#ifdef PROV_WIFI
  /**
   * Process captive portal loop
   */
  void handleCaptivePortal();

  /**
   * Setup captive portal
   */
  void setupCaptivePortal();

  /**
   * Save credentials of setup WiFi connection to LittleFS/EEPROM
   */
  void saveCaptivePortalWifi();

  /**
   * Save parameters (WS Token, domain) to LittleFS/EEPROM
   */
  void saveCaptivePortalParameters();

  /**
   * Disable captive portal timeout before OTA update
   */
  void preOtaUpdateCallback();

  std::unique_ptr<WiFiManager> wifi_manager_;
  std::unique_ptr<WiFiManagerParameter> ws_token_parameter_;
  static const __FlashStringHelper* ws_token_placeholder_;
  std::unique_ptr<WiFiManagerParameter> core_domain_parameter_;
  std::unique_ptr<WiFiManagerParameter> secure_url_parameter_;
  bool disable_captive_portal_timeout_ = false;
#endif
#ifdef DEVICE_TYPE_FIRE_DATA_LOGGER
  std::shared_ptr<PCA9536D> input_bank_;
#endif

  ServiceGetters services_;
  std::shared_ptr<BleServer> ble_server_;
  std::shared_ptr<WiFiNetwork> wifi_network_;
#ifdef GSM_NETWORK
  std::shared_ptr<GsmNetwork> gsm_network_;
#endif
  std::shared_ptr<WebSocket> web_socket_;

  Mode mode_ = Mode::ConnectWiFi;
  UseNetwork use_network_ = UseNetwork::kNone;
  std::chrono::steady_clock::time_point mode_start_;

  /// Set true once WebSocket connects. Will not set false on disconnect. Avoids
  /// starting captive portal during normal operation. Only on boot
  bool web_socket_connected_since_boot_ = false;

  std::chrono::steady_clock::time_point wifi_connect_start_ =
      std::chrono::steady_clock::time_point::min();

  /// Last time the internet time was checked
  std::chrono::steady_clock::time_point last_time_check_ =
      std::chrono::steady_clock::time_point::max();
  const std::chrono::steady_clock::duration time_check_period_ =
      std::chrono::hours(24);

  /// Last time the GSM/WiFi switch was checked
  std::chrono::steady_clock::time_point last_gsm_wifi_switch_check_ =
      std::chrono::steady_clock::time_point::min();
  const std::chrono::steady_clock::duration gsm_wifi_switch_check_period_ =
      std::chrono::seconds(2);
};

}  // namespace connectivity
}  // namespace tasks
}  // namespace inamata
