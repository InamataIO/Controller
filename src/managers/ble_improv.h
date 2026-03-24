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

    WiFiScanAP(const String& ssid = "", const int32_t rssi = 0,
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
  void onWrite(NimBLECharacteristic* characteristic,
               NimBLEConnInfo& connInfo) final;

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
  void setServerAuth(const improv::ImprovCommand& command);

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

  /**
   * Handles user sending custom data
   */
  void setUserData(const improv::ImprovCommand& command);

#ifdef GSM_NETWORK
  /**
   * Handles returning the device's state incl. ICCID and IMEI
   */
  void sendMobileStateResponse();

  /**
   * Starts scanning for available mobile networks
   *
   * Optional mobile technology parameter. Valid options: auto, gsm, lte
   *
   * \param command Improv command with search params in SSID field
   */
  void startGetMobileNetworks(const improv::ImprovCommand& command);

  /**
   * Check if mobile networks scan finished, then send BLE response for mobile
   * networks
   */
  void handleGetMobileOperators();

  /**
   * List of mobile operators the device is allowed to connect to
   *
   * The data format of the operator list is: <op1,op2,op3,...> where op_n is an
   * MCC/MNC tuple. MCC is the mobile country code, a 3 digit code for each
   * country, and MNC is the mobile network code, a 2-3 digit code for each
   * operator in a country. One point, MNC 01 and 001 are not the same and both
   * valid.
   *
   * The SSID payload of an example command in South Africa would be:
   *
   *     65501,65510
   *
   * This would correspond to Vodacom and MTN. For vodavom in Lesotho, the
   * MCC/MNC tuple would be:
   *
   *     65101
   *
   * \param command Improv command with SSID field containing the MNC/MCC tuple
   */
  void setAllowedMobileOperators(const improv::ImprovCommand& command);

  /// Whether X_GET_MOBILE_OPERATORS command is active
  bool scan_mobile_operators_ = false;
#endif

  NimBLEService* ble_improv_service_{nullptr};
  NimBLECharacteristic* ble_status_char_{nullptr};
  NimBLECharacteristic* ble_error_char_{nullptr};
  NimBLECharacteristic* ble_rpc_command_char_{nullptr};
  NimBLECharacteristic* ble_rpc_response_char_{nullptr};
  NimBLECharacteristic* ble_capabilities_char_{nullptr};

  ServiceGetters services_;

  // RPC frame bytes of length n:
  // 1: Command type
  // 2: Data length
  // 3 to n-1: Data bytes
  // n: Checksum
  // https://www.improv-wifi.com/ble/
  std::vector<uint8_t> rpc_data_;

  String user_data_;

  /// Whether GET_WIFI_NETWORKS command is active
  bool scan_wifi_aps_ = false;

  /// Verify WIFI_SETTINGS command and save AP details on success
  WiFiAP wifi_ap_;
  std::chrono::steady_clock::time_point wifi_connect_start_ =
      std::chrono::steady_clock::time_point::min();
  std::chrono::milliseconds wifi_connect_timeout_ = std::chrono::seconds(30);

  improv::State state_ = improv::STATE_STOPPED;
  improv::Error error_ = improv::ERROR_NONE;
};

}  // namespace inamata