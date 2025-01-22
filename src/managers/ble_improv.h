#pragma once

#include <improv.h>

#include "managers/service_getters.h"
#include "managers/services.h"

namespace inamata {

class BleImprov : public NimBLECharacteristicCallbacks {
 public:
  struct WiFiScanAP {
    String ssid;
    int32_t rssi;
    bool auth_required;

    WiFiScanAP(const String &ssid = "", const int32_t rssi = 0,
               bool auth_required = false)
        : ssid(ssid), rssi(rssi), auth_required(auth_required) {}
  };

  BleImprov(ServiceGetters services);
  virtual ~BleImprov() = default;

  /**
   * Periodically called to handle setup, incoming data and state changes
   */
  void handle();

  /**
   * Shutdown the BLE improv service. Can delete object afterwards.
   */
  void stop();

  /**
   * Publish state on BLE service characteristic
   */
  void setState(improv::State state);

  /**
   * Get state
   */
  const improv::State getState() const;

  /**
   * Publish error on BLE service characteristic
   */
  void setError(improv::Error error);

  /**
   * Callback when receiving BLE RCP data
   *
   * \param characteristic The BLE char that received the data
   */
  void onWrite(NimBLECharacteristic *characteristic) final;

 private:
  /**
   * Set up BLE service
   */
  void setupService();

  /**
   * Handle received RPC data (set WiFi AP, identify, ...)
   */
  void processRpcData();

  /**
   * Handle X_SET_SERVER_AUTH
   */
  void handleSetServerAuth(const improv::ImprovCommand &command);

  /**
   * Check if connecting to WiFi AP for provisioning has timed out
   */
  void handleWiFiConnectTimeout();

  /**
   * Send response to BLE client to finish provisioning
   */
  void sendProvisionedResponse();

  /**
   * Send BLE response for device info (firmware variant + version)
   */
  void sendDeviceInfoResponse();

  /**
   * Send BLE response for device type (UUID)
   */
  void sendDeviceTypeResponse();

  void startGetWifiNetworks();

  /**
   * Check if WiFi scan finished, then send BLE response for wifi networks
   */
  void handleGetWifiNetworks();

  NimBLEService *ble_improv_service_{nullptr};
  NimBLECharacteristic *ble_status_char_{nullptr};
  NimBLECharacteristic *ble_error_char_{nullptr};
  NimBLECharacteristic *ble_rpc_command_char_{nullptr};
  NimBLECharacteristic *ble_rpc_response_char_{nullptr};
  NimBLECharacteristic *ble_capabilities_char_{nullptr};

  ServiceGetters services_;

  // RPC frame bytes of length n:
  // 1: Command type
  // 2: Data length
  // 3 to n-1: Data bytes
  // n: Checksum
  // https://www.improv-wifi.com/ble/
  std::vector<uint8_t> rpc_data_;

  // Whether GET_WIFI_NETWORKS command is active
  bool scan_wifi_aps_ = false;

  // Verify WIFI_SETTINGS command and save AP details on success
  WiFiAP wifi_ap_;
  std::chrono::steady_clock::time_point wifi_connect_start_ =
      std::chrono::steady_clock::time_point::min();
  std::chrono::milliseconds wifi_connect_timeout_ = std::chrono::seconds(30);

  improv::State state_ = improv::STATE_STOPPED;
  improv::Error error_ = improv::ERROR_NONE;
};

}  // namespace inamata