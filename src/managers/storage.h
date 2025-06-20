#pragma once

#include <ArduinoJson.h>
#include <LittleFS.h>

#include "managers/logging.h"
#include "managers/types.h"

namespace inamata {

class Storage {
 public:
  void openFS();
  void closeFS();

  /**
   * Recursively deletes all files and the directory itself
   *
   * \param path Directory/file to be deleted
   */
  static void recursiveRm(const char* path);

  /**
   * Load stored secrets into passed JSON doc
   *
   * \param secrets_doc The JSON doc to load secrets into
   * \return If an error occured during deserialization. No error if no file
   */
  ErrorResult loadSecrets(JsonDocument& secrets_doc);

  /**
   * Store secrets JSON doc on the file system
   *
   * \param secrets_doc JSON doc with the secrets to be stored
   */
  ErrorResult storeSecrets(JsonVariantConst secrets);

  /**
   * Save WiFi AP credentials to secret store
   *
   * \param wifi_ap The WiFi AP credentials to store
   * \return If an error occured
   */
  ErrorResult saveWiFiAP(const WiFiAP& wifi_ap);

  /**
   * Set the components of the WebSocket server URL
   *
   * \param domain Domain incl. subdomain of the server to connect to
   * \param path Path in the URL to send HTTP request to
   * \param secure_url Whether to use TLS encryption
   */
  ErrorResult saveWsUrl(const char* domain, const char* path = nullptr,
                        bool secure_url = true);

  /**
   * Delete components of stored WebSocket server URL
   */
  ErrorResult deleteWsUrl();

  /**
   * Save server auth token to secret store
   *
   * \param token The WiFi AP credentials to store
   * \return If an error occured
   */
  ErrorResult saveAuthToken(const char* token);

  /**
   * Load saved peripherals to JSON doc
   *
   * \return Error if one occured
   */
  ErrorResult loadPeripherals(JsonDocument& peripheral_doc);

  /**
   * Store passed peripherals or private JSON doc on flash storage
   *
   * \return Error if one occured
   */
  ErrorResult storePeripherals(JsonArrayConst peripherals);

  /**
   * Saves a single peripheral to flash storage
   *
   * \param doc The peripheral to be saved. Overwrites old version if it exists
   * \return If an error occured
   */
  ErrorResult savePeripheral(const JsonObjectConst& peripheral);

  /**
   * Deletes stored peripherals
   */
  void deletePeripherals();

  /**
   * Removes a single peripheral from flash storage
   *
   * \param peripheral_id The ID of the peripheral
   */
  void deletePeripheral(const char* peripheral_id);

  ErrorResult loadBehavior(JsonDocument& behavior);
  ErrorResult storeBehavior(const JsonObjectConst& behavior);
  void deleteBehavior();

  ErrorResult loadCustomConfig(JsonDocument& config);
  ErrorResult storeCustomConfig(const JsonObjectConst& config);
  void deleteCustomConfig();

  static const char* arduino_board_;
  static const char* device_type_name_;
  static const char* device_type_id_;

  static const char* core_domain_key_;
  static const char* ws_url_path_key_;
  static const char* secure_url_key_;
  static const char* ws_token_key_;

  static const char* wifi_aps_key_;
  static const char* wifi_ap_ssid_key_;
  static const char* wifi_ap_password_key_;

 private:
  static const char* secrets_path_;
  static const char* peripherals_path_;
  static const char* behavior_path_;
  static const char* custom_config_path_;
  static const char* type_;
};

}  // namespace inamata
