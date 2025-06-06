#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include <WebSocketsClient.h>

#include <functional>
#include <map>
#include <vector>

#include "NetworkClient.h"
#include "configuration.h"
#include "managers/logging.h"
#include "managers/types.h"
#include "utils/uuid.h"
#include "utils/value_unit.h"

namespace inamata {

/**
 * Connection to the server via WebSockets
 *
 * This class creates a bi-direactional connection with the server to create
 * peripheral and tasks on the controller, and return their output to the
 * server.
 */
class WebSocket {
 public:
  enum class ConnectState { kConnected, kConnecting, kFailed };

  using Callback = std::function<void(const JsonObjectConst& message)>;
  using CallbackMap = std::map<String, Callback>;

  struct Config {
    Callback action_controller_callback;
    Callback behavior_controller_callback;
    std::function<void(JsonObject)> set_behavior_register_data;
    std::function<std::vector<utils::VersionedID>()> get_peripheral_ids;
    Callback peripheral_controller_callback;
    std::function<std::vector<utils::UUID>()> get_task_ids;
    Callback task_controller_callback;
    Callback lac_controller_callback;
    Callback ota_update_callback;
    const char* core_domain;
    const char* ws_url_path;
    const char* ws_token;
    bool secure_url;
  };

  /**
   * Connection to the Inamata server over websockets.
   *
   * This enables bi-directional communication between the controller and the
   * server while removing the intermediate such as the Coordinator over MQTT.
   *
   * \param config Copies out the config values to perform its initialization
   */
  WebSocket(const Config& config);
  virtual ~WebSocket() = default;

  const String& type();

  ConnectState handle();

  /**
   * Resets the last connect time to start a new connect attempt
   */
  void resetConnectAttempt();

  void sendTelemetry(JsonObject data, const utils::UUID* task_id = nullptr,
                     const utils::UUID* lac_id = nullptr);
  /**
   * Make a JSON object with the value units and UUID from the peripheral
   *
   * \param[in] values The measured values and data point types
   * \param[in] peripheral_id The ID of the peripheral that returned the values
   * \param[in] is_fixed Whether fixed peripheral and DPT IDs are used
   * \param[out] telemetry The JSON object to add the value units and UUID to
   */
  static void packageTelemetry(const std::vector<utils::ValueUnit>& values,
                               const utils::UUID& peripheral_id,
                               const bool is_fixed, JsonObject& telemetry);

  void sendLimitEvent(JsonObject data);
  void sendBootErrors();
  void sendRegister();
  void sendError(const String& who, const String& message);
  void sendError(const ErrorResult& error, const String& request_id = "");

  void sendDebug(const String& message);

  void sendResults(JsonObjectConst results);
  static void addResultEntry(const String& uuid, const ErrorResult& error,
                             const JsonArray& results);

  void sendSystem(JsonObject data);

  void resetUrl();
  void setUrl(const char* domain, const char* path = nullptr,
              bool secure_url = true);
  void setWsToken(const char* token);
  const bool isWsTokenSet() const;

  void setSentMessageCallback(std::function<void()> callback);
  void clearSentMessageCallback();

  /**
   * Checks if the WebSocket connected to the server
   *
   * @return true if connected
   */
  bool isConnected();

  static const __FlashStringHelper* firmware_version_;
  String core_domain_;
  String ws_url_path_;
  bool secure_url_;

  static const __FlashStringHelper* request_id_key_;
  static const __FlashStringHelper* type_key_;

  static const __FlashStringHelper* limit_event_type_;
  static const __FlashStringHelper* result_type_;
  static const __FlashStringHelper* telemetry_type_;

  /// Controller action object in command messages
  static const __FlashStringHelper* action_key_;
  static const __FlashStringHelper* behavior_key_;
  static const __FlashStringHelper* task_key_;
  static const __FlashStringHelper* system_type_;
  static const __FlashStringHelper* lac_key_;

  // Keys used in telemetry messages
  static const __FlashStringHelper* telemetry_peripheral_key_;
  static const __FlashStringHelper* fixed_peripheral_key_;

  // Keys and names used by result messages
  static const __FlashStringHelper* uuid_key_;
  static const __FlashStringHelper* result_status_key_;
  static const __FlashStringHelper* result_detail_key_;
  static const __FlashStringHelper* result_success_name_;
  static const __FlashStringHelper* result_fail_name_;
  /// Used by LACs to tell if they are running, installed or so
  static const __FlashStringHelper* result_state_key_;

  // Limit keys
  static const __FlashStringHelper* limit_id_key_;
  static const __FlashStringHelper* fixed_peripheral_id_key_;
  static const __FlashStringHelper* fixed_dpt_id_key_;

 private:
  /**
   * Perform setup and check if connect timeout has been reached
   *
   * @return ConnectState kConnecting if retrying, kFailed if timed out
   */
  ConnectState connect();

  void handleEvent(WStype_t type, uint8_t* payload, size_t length);
  void handleData(const uint8_t* payload, size_t length);

  /**
   * Save the up/down durations and timepoints when the connection state changes
   *
   * @param is_connected True if the connection is currently connected
   */
  void updateUpDownTime(const bool is_connected);
  void sendUpDownTimeData();

  /**
   * Send JSON data to the server
   *
   * Calculate the size of the resultant serialized JSON, create a buffer of
   * that size and serialize the JSON into that buffer.
   * Add extra byte for the null terminator
   *
   * @param doc JSON data to be sent
   */
  void sendJson(JsonVariantConst doc);

  bool is_setup_ = false;

  /// Whether the WebSocket was connected during the last check
  bool was_connected_ = false;
  bool send_on_connect_messages_ = false;
  /// The timepoint when the last connect attempt started
  std::chrono::steady_clock::time_point last_connect_start_ =
      std::chrono::steady_clock::time_point::min();
  /// The timepoint when the connection last went up
  std::chrono::steady_clock::time_point last_connect_up_ =
      std::chrono::steady_clock::time_point::min();
  /// The timepoint when the connection last went down
  std::chrono::steady_clock::time_point last_connect_down_ =
      std::chrono::steady_clock::time_point::min();
  /// The duration of the last WebSocket connection uptime
  std::chrono::steady_clock::duration last_up_duration_ =
      std::chrono::steady_clock::duration::min();
  /// The duration of the last WebSocket connection downtime
  std::chrono::steady_clock::duration last_down_duration_ =
      std::chrono::steady_clock::duration::min();

  Callback action_controller_callback_;
  Callback behavior_controller_callback_;
  std::function<void(JsonObject)> set_behavior_register_data_;
  std::function<std::vector<utils::VersionedID>()> get_peripheral_ids_;
  Callback peripheral_controller_callback_;
  std::function<std::vector<utils::UUID>()> get_task_ids_;
  Callback task_controller_callback_;
  Callback lac_controller_callback_;
  Callback ota_update_callback_;

  std::function<void()> sent_message_callback_;

  String ws_token_;
  static const __FlashStringHelper* default_core_domain_;
  static const __FlashStringHelper* default_ws_url_path_;
};

}  // namespace inamata