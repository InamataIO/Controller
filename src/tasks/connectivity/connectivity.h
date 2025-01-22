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

namespace inamata {
namespace tasks {
namespace connectivity {

class CheckConnectivity : public BaseTask {
 public:
  enum class Mode {
    ConnectWiFi,
    ProvisionDevice,
  };

  CheckConnectivity(const ServiceGetters& services, Scheduler& scheduler);
  virtual ~CheckConnectivity();

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
  void handleClockSync();

  /**
   * Performs WebSocket processing and ensure connected state
   *
   * If WebSocket connection fails after a timeout, open a captive portal
   */
  void handleWebSocket();

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

  ServiceGetters services_;
  std::shared_ptr<Network> network_;
  std::shared_ptr<WebSocket> web_socket_;

  Mode mode_ = Mode::ConnectWiFi;
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
};

}  // namespace connectivity
}  // namespace tasks
}  // namespace inamata
