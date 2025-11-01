#pragma once

#include <TaskSchedulerDeclarations.h>

#include <chrono>
#include <limits>

#include "managers/ble_improv.h"
#include "managers/service_getters.h"
#include "managers/web_socket.h"
#include "tasks/base_task.h"
#include "utils/chrono.h"
#ifdef DEVICE_TYPE_FIRE_DATA_LOGGER
#include "peripheral/peripherals/digital_in/digital_in.h"
#endif

namespace inamata {
namespace tasks {
namespace connectivity {

class CheckConnectivity : public BaseTask {
 public:
#ifdef DEVICE_TYPE_FIRE_DATA_LOGGER
  using DigitalIn = peripheral::peripherals::digital_in::DigitalIn;
#endif

  enum class Mode {
    ConnectWiFi,
#ifdef GSM_NETWORK
    ConnectGsm,
#endif
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

  enum class WiFiScanMode { kNone, kScanning, kFinished };

  void handleBleServer();
  void handleImprov();

  std::unique_ptr<BleImprov> improv_;

#ifdef DEVICE_TYPE_FIRE_DATA_LOGGER
  std::shared_ptr<DigitalIn> gsm_wifi_toggle_;
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
