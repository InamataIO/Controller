#pragma once

#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <esp_https_ota.h>

#include "managers/ble_server.h"

// Include ble_server.h first to avoid LOG_LEVEL redefinitions

#ifdef GSM_NETWORK
#include "managers/gsm_https_client.h"
#endif
#include "managers/service_getters.h"
#include "tasks/base_task.h"
#include "utils/error_store.h"

namespace inamata {

/**
 * Service to perform OTA updates
 */
class OtaUpdater : public tasks::BaseTask {
 public:
  enum class Network { kWifi, kGsm };

  OtaUpdater(Scheduler& scheduler);
  virtual ~OtaUpdater() = default;

  const String& getType() const final;
  static const String& type();

  void setServices(ServiceGetters services);

  void useNetwork(Network network);

  /**
   * Handle the command to update the firmware
   *
   * \param message Information regarding the firmware update
   */
  void handleCallback(const JsonObjectConst& message);

  /**
   * Acquire a lock to prevent parallel updates
   */
  bool OnTaskEnable();

  /**
   * Download the firmware image as a stream and write it to the flash
   */
  bool TaskCallback();

  /**
   * Perform clean up for next OTA update attempt
   *
   * Free the buffer and release a lock to allow the next update request.
   */
  void OnTaskDisable();

 private:
  void sendResult(const char* status, const String& error = "",
                  const char* request_id = nullptr);

  void clearClients();

  /// The server to which to reply to
  ServiceGetters services_;

  /// Which network to use (WiFi / GSM)
  Network network_ = Network::kWifi;

  /// The request ID used by the update command
  String request_id_;

  /// A lock when an update is in progress
  bool is_updating_ = false;

  /// The last update progress percentage
  int last_percent_update = -1;

  /// Whether to restart on success. Ignored on update fail
  bool restart_;

  /// Size of the image to be downloaded
  int32_t image_size_ = 0;
  std::vector<uint8_t> buffer_;

  std::unique_ptr<WiFiClientSecure> wifi_client_;
  std::unique_ptr<HTTPClient> wifi_http_client_;
#ifdef GSM_NETWORK
  std::unique_ptr<GsmHttpsClient> gsm_https_client_;
#endif

  esp_http_client_config_t ota_http_config_;
  esp_https_ota_config_t ota_config_;
  esp_https_ota_handle_t ota_handle_;

  static constexpr std::chrono::milliseconds default_interval_{50};

  static const char* update_command_key_;
  static const char* url_key_;
  static const char* image_size_key_;
  static const char* md5_hash_key_;
  static const char* restart_key_;

  static const char* status_key_;
  static const char* status_start_;
  static const char* status_updating_;
  static const char* status_finish_;
  static const char* status_fail_;
  static const char* detail_key_;
  static const char* failed_to_connect_error_;
  static const char* connection_lost_error_;
  static const char* update_in_progress_error_;
  static const char* http_code_error_;
  static const char* http_connect_error_;
  static const char* size_mismatch_;
};

}  // namespace inamata
