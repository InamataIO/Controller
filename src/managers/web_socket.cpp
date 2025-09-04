#include "web_socket.h"

#include <esp_tls.h>

#include "managers/services.h"
#include "utils/chrono.h"

namespace inamata {

WebSocketsClient websocket_client;
using namespace std::placeholders;

WebSocket::WebSocket(const WebSocket::Config& config)
    : core_domain_(config.core_domain),
      ws_url_path_(config.ws_url_path),
      secure_url_(config.secure_url),
      action_controller_callback_(config.action_controller_callback),
      behavior_controller_callback_(config.behavior_controller_callback),
      set_behavior_register_data_(config.set_behavior_register_data),
      get_peripheral_ids_(config.get_peripheral_ids),
      peripheral_controller_callback_(config.peripheral_controller_callback),
      get_task_ids_(config.get_task_ids),
      task_controller_callback_(config.task_controller_callback),
      lac_controller_callback_(config.lac_controller_callback),
      ota_update_callback_(config.ota_update_callback) {
  if (core_domain_.isEmpty()) {
    core_domain_ = default_core_domain_;
    secure_url_ = true;
  }
  if (ws_url_path_.isEmpty()) {
    ws_url_path_ = default_ws_url_path_;
  }
  if (config.ws_token) {
    setWsToken(config.ws_token);
  }
}

const String& WebSocket::type() {
  static const String name{"WebSocket"};
  return name;
}

void WebSocket::setSentMessageCallback(std::function<void()> callback) {
  sent_message_callback_ = callback;
}

void WebSocket::clearSentMessageCallback() { sent_message_callback_ = nullptr; }

bool WebSocket::isConnected() {
  const bool is_connected = websocket_client.isConnected();
  updateUpDownTime(is_connected);
  return is_connected;
}

WebSocket::ConnectState WebSocket::connect() {
  // Configure the WebSocket interface with the server, TLS certificate and the
  // reconnect interval
  if (!is_setup_) {
    is_setup_ = true;
    TRACEF("Connect: %s:%d%s : %s\r\n", core_domain_.c_str(),
           secure_url_ ? 443 : 8000, ws_url_path_.c_str(), ws_token_.c_str());
    // WS token has to be set in LittleFS, EEPROM or via captive portal
    if (!isWsTokenSet()) {
      TRACELN(F("ws_token not set"));
      return ConnectState::kFailed;
    }
    TRACEF("Connecting: %s, %s, %d, %s\r\n", core_domain_.c_str(),
           ws_url_path_.c_str(), secure_url_, ws_token_.c_str());
    if (secure_url_) {
      websocket_client.beginSslWithBundle(
          core_domain_.c_str(), 443, ws_url_path_.c_str(),
          rootca_crt_bundle_start, ws_token_.c_str());
    } else {
      websocket_client.begin(core_domain_.c_str(), 8000, ws_url_path_.c_str(),
                             ws_token_.c_str());
    }
    websocket_client.onEvent(
        std::bind(&WebSocket::handleEvent, this, _1, _2, _3));
    websocket_client.setReconnectInterval(5000);
  }

  if (last_connect_start_ == last_connect_start_.min()) {
    last_connect_start_ = std::chrono::steady_clock::now();
  }

  // If timed out, say failed, else attempt to connect
  if (std::chrono::steady_clock::now() - last_connect_start_ >
      web_socket_connect_timeout) {
    return ConnectState::kFailed;
  }
  websocket_client.loop();
  return ConnectState::kConnecting;
}

WebSocket::ConnectState WebSocket::handle() {
  if (isConnected()) {
    // On reconnect, send register and other messages
    if (send_on_connect_messages_) {
      TRACELN(F("Reconnected to server"));
      send_on_connect_messages_ = false;
      sendRegister();
      sendUpDownTimeData();
    }
    websocket_client.loop();
    handleRetryBuffer();
    return ConnectState::kConnected;
  }

  return connect();
}

void WebSocket::resetConnectAttempt() {
  last_connect_start_ = last_connect_start_.min();
}

void WebSocket::sendTelemetry(JsonObject data, const utils::UUID* task_id,
                              const utils::UUID* lac_id) {
  // Only retry if time has been set, as else outdated server time is used
  bool retry = false;
  if (!data[time_key_].isNull()) {
    retry = true;
  } else {
    TRACELN("Time not set for telemetry message");
  }

  data[WebSocket::type_key_] = WebSocket::telemetry_type_;

  // Set task and LAC IDs if given to be able to reference initiator
  if (task_id && task_id->isValid()) {
    data[WebSocket::task_key_] = task_id->toString();
  }
  if (lac_id && lac_id->isValid()) {
    data[WebSocket::lac_key_] = lac_id->toString();
  }
  sendJson(data, retry);
}

void WebSocket::packageTelemetry(const std::vector<utils::ValueUnit>& values,
                                 const utils::UUID& peripheral_id,
                                 const bool is_fixed, JsonObject& telemetry) {
  // Create an array for the value units and get them from the peripheral
  JsonArray value_units_doc =
      telemetry[utils::ValueUnit::data_points_key].to<JsonArray>();

  // Create a JSON object representation for each value unit in the array
  const char* dpt_key = is_fixed ? utils::ValueUnit::fixed_data_point_type_key
                                 : utils::ValueUnit::data_point_type_key;
  for (const auto& value_unit : values) {
    JsonObject value_unit_object = value_units_doc.add<JsonObject>();
    value_unit_object[utils::ValueUnit::value_key] = value_unit.value;
    value_unit_object[dpt_key] = value_unit.data_point_type.toString();
  }

  // Add the peripheral UUID to the result. Fixed peripherals use a different
  // key to allow the server to map to the generated peripheral
  const char* peripheral_key =
      is_fixed ? fixed_peripheral_key_ : telemetry_peripheral_key_;
  telemetry[peripheral_key] = peripheral_id.toString();

  // Set the time
  if (Services::is_time_synced_) {
    telemetry[time_key_] = utils::getIsoTimestamp();
  }
}

void WebSocket::sendLimitEvent(JsonObject data) {
  data[WebSocket::type_key_] = WebSocket::limit_event_type_;
  bool retry = false;
  if (!data[WebSocket::time_key_].isNull()) {
    retry = true;
  }
  sendJson(data, retry);
}

void WebSocket::sendBootErrors() {
  JsonObjectConst empty_message;
  peripheral_controller_callback_(empty_message);
}

void WebSocket::sendRegister() {
  JsonDocument doc_out;
  JsonObject register_obj = doc_out.to<JsonObject>();

  // Use the register message type
  register_obj["type"] = "reg";

  // Set the firmware version number
  register_obj["version"] = firmware_version_;

  // Collect all added peripheral ids and write them to a JSON doc
  if (behavior_based) {
    set_behavior_register_data_(register_obj);
  } else {
    std::vector<utils::VersionedID> pvids = get_peripheral_ids_();
    if (!pvids.empty()) {
      JsonArray pvs = register_obj[F("pvs")].to<JsonArray>();
      for (const auto& pvid : pvids) {
        JsonObject pv = pvs.add<JsonObject>();
        pv["id"] = pvid.id.toString();
        pv["v"] = pvid.version;
      }
    }
  }

  // Collect all running task ids and write them to a JSON doc
  std::vector<utils::UUID> task_ids = get_task_ids_();
  if (!task_ids.empty()) {
    JsonArray tasks = register_obj[F("tasks")].to<JsonArray>();
    for (const auto& task_id : task_ids) {
      if (task_id.isValid()) {
        tasks.add(task_id.toString());
      }
    }
  }

  sendJson(register_obj);
}

void WebSocket::sendError(const String& who, const String& message) {
  JsonDocument doc_out;

  // Use ther error message type
  doc_out["type"] = "err";

  // Place the error message
  doc_out["message"] = message.c_str();

  sendJson(doc_out);
}

void WebSocket::sendError(const ErrorResult& error, const String& request_id) {
  JsonDocument doc_out;

  // Use ther error message type
  doc_out["type"] = "err";

  // Where the message originated from
  doc_out["context"] = error.who_.c_str();

  // The error itself
  doc_out["message"] = error.detail_.c_str();

  // The request ID to enable tracing
  doc_out["request_id"] = request_id.c_str();

  sendJson(doc_out);
}

void WebSocket::sendDebug(const String& message) {
  if (!websocket_client.isConnected()) {
    return;
  }
  JsonDocument doc_out;

  // Use ther error message type
  doc_out["type"] = "dbg";

  // The error itself
  doc_out["message"] = message.c_str();

  sendJson(doc_out);
}

void WebSocket::sendResults(JsonObjectConst results) {
  if (!websocket_client.isConnected()) {
    return;
  }
  sendJson(results);
}

void WebSocket::addResultEntry(const String& uuid, const ErrorResult& error,
                               const JsonArray& results) {
  JsonObject result = results.add<JsonObject>();

  // Save whether the task could be started or the reason for failing
  if (error.isError()) {
    result[uuid_key_] = uuid;
    result[result_status_key_] = result_fail_name_;
    result[result_detail_key_] = error.detail_;
  } else {
    result[uuid_key_] = uuid;
    result[WebSocket::result_status_key_] = WebSocket::result_success_name_;
  }
}

void WebSocket::sendSystem(JsonObject data) {
  if (!websocket_client.isConnected()) {
    return;
  }
  data[WebSocket::type_key_] = WebSocket::system_type_;
  sendJson(data);
}

void WebSocket::resetUrl() {
  is_setup_ = false;
  core_domain_ = default_core_domain_;
  ws_url_path_ = default_ws_url_path_;
  secure_url_ = true;
}

void WebSocket::setUrl(const char* domain, const char* path, bool secure_url) {
  if (domain && strlen(path) >= 1) {
    core_domain_ = domain;
  } else {
    core_domain_ = default_core_domain_;
  }
  if (path && strlen(path) >= 1) {
    ws_url_path_ = path;
  } else {
    ws_url_path_ = default_ws_url_path_;
  }
  secure_url_ = secure_url;
}

void WebSocket::setWsToken(const char* ws_token) {
  const char* token_prefix = "token_";
  is_setup_ = false;
  ws_token_.clear();
  ws_token_.reserve(strlen(token_prefix) + strlen(ws_token));
  ws_token_ = token_prefix;
  ws_token_ += ws_token;
  TRACEF("Set token: %s\r\n", ws_token_.c_str());
}

const bool WebSocket::isWsTokenSet() const { return !ws_token_.isEmpty(); }

void WebSocket::handleEvent(WStype_t type, uint8_t* payload, size_t length) {
  // Print class type before the printing the message type
  switch (type) {
    case WStype_DISCONNECTED: {
      TRACELN(F("WS disconnected"));
    } break;
    case WStype_CONNECTED: {
      TRACEF("Connected to: %s\r\n", reinterpret_cast<char*>(payload));
    } break;
    case WStype_TEXT: {
      TRACEF("Got text %u: %s\r\n", length, reinterpret_cast<char*>(payload));
      handleData(payload, length);
    } break;
    case WStype_PING:
      TRACELN(F("Received ping"));
      break;
    case WStype_BIN:
    case WStype_ERROR:
    case WStype_FRAGMENT_TEXT_START:
    case WStype_FRAGMENT_BIN_START:
    case WStype_FRAGMENT:
    case WStype_FRAGMENT_FIN:
    case WStype_PONG:
      TRACEF("Unhandled message type: %u\r\n", type);
      break;
  }
}

void WebSocket::handleData(const uint8_t* payload, size_t length) {
  // Deserialize the JSON object into allocated memory
  JsonDocument doc_in;
  const DeserializationError error = deserializeJson(doc_in, payload, length);
  if (error) {
    sendError(type(), String(F("Deserialize failed: ")) + error.c_str());
    return;
  }

  // Pass the message to the handlers
  JsonObjectConst message = doc_in.as<JsonObjectConst>();
  action_controller_callback_(message);
  if (behavior_controller_callback_) {
    behavior_controller_callback_(message);
  }
  peripheral_controller_callback_(message);
  task_controller_callback_(message);
  lac_controller_callback_(message);
  if (ota_update_callback_) {
    ota_update_callback_(message);
  }
}

void WebSocket::updateUpDownTime(const bool is_connected) {
  if (is_connected != was_connected_) {
    was_connected_ = is_connected;
    const auto now = std::chrono::steady_clock::now();
    if (is_connected) {
      TRACELN(F("WS connected"));
      // Connection went up
      if (last_connect_down_ != std::chrono::steady_clock::time_point::min()) {
        // Only update if last_connect_down_ has been set
        last_down_duration_ = now - last_connect_down_;
      }
      last_connect_up_ = now;
      send_on_connect_messages_ = true;
    } else {
      // Connection went down
      if (last_connect_up_ != std::chrono::steady_clock::time_point::min()) {
        // Only update if last_connect_up_ has been set
        last_up_duration_ = now - last_connect_up_;
      }
      last_connect_down_ = now;
    }
  }
}

void WebSocket::sendUpDownTimeData() {
  // Only send after valid times have been recorded
  if (last_up_duration_ != std::chrono::steady_clock::duration::min() &&
      last_down_duration_ != std::chrono::steady_clock::duration::min()) {
    JsonDocument doc_out;
    if (last_up_duration_ != std::chrono::steady_clock::duration::min()) {
      // If the up duration is valid, print and send the data
      int64_t last_up_duration_s =
          std::chrono::duration_cast<std::chrono::seconds>(last_up_duration_)
              .count();
      doc_out[F("last_ws_up_duration_s")] = last_up_duration_s;
    }
    if (last_down_duration_ != std::chrono::steady_clock::duration::min()) {
      // If the down duration is valid, print and send the data
      doc_out[F("last_ws_down_duration_s")] =
          std::chrono::duration_cast<std::chrono::seconds>(last_down_duration_)
              .count();
    }
    sendSystem(doc_out.as<JsonObject>());
  }
}

void WebSocket::sendJson(JsonVariantConst doc, const bool retry) {
  std::vector<char> buffer = std::vector<char>(measureJson(doc) + 1);
  size_t n = serializeJson(doc, buffer.data(), buffer.size());
  TRACELN(buffer.data());
  const bool success = websocket_client.sendTXT(buffer.data(), n);
  if (!success) {
    if (retry) {
      saveToRetryBuffer(buffer);
    } else {
      TRACELN("Failed sending");
    }
  } else {
    if (sent_message_callback_) {
      sent_message_callback_();
    }
  }
}

void WebSocket::handleRetryBuffer() {
  if (retry_queue_.empty()) {
    return;
  }

  RetryMessage& retry_message = retry_queue_.back();
  // Check if retry limit exceeded. Delete item if retries exceeded
  if (!canRetryMessage(retry_message)) {
    retry_queue_.pop_back();
    return;
  }

  // Try to send the failed message
  const bool success = websocket_client.sendTXT(
      retry_message.message.data(), retry_message.message.size() - 1);
  if (success) {
    Serial.println("Sent retry message");
    retry_queue_.pop_back();
    return;
  }

  // Retry failed so queue the message again if possible
  retry_message.tries++;
  if (canRetryMessage(retry_message)) {
    Serial.println("Requeueing retry message");
    retry_queue_.push_front(std::move(retry_queue_.back()));
    retry_queue_.pop_back();
  } else {
    Serial.println("Dropping retry message");
    retry_queue_.pop_back();
  }
  return;
}

void WebSocket::saveToRetryBuffer(std::vector<char>& buffer) {
  bool pop_back = false;
  if (retry_queue_.size() >= kMaxRetryMessages_) {
    retry_queue_.pop_back();
    pop_back = true;
  }
  TRACEF("Saving to retry buffer (%d:%d)\r\n", retry_queue_.size(), pop_back);
  retry_queue_.emplace_front(buffer, std::chrono::steady_clock::now(), 0);
}

bool WebSocket::canRetryMessage(const RetryMessage& message) {
  if (message.message.size() <= 0) {
    return false;
  }
  if (message.tries > kMaxRetrySendAttempts_) {
    return false;
  }
  if (utils::chrono_abs(std::chrono::steady_clock::now() - message.created_at) >
      kMaxRetryMessageAge_) {
    return false;
  }
  return true;
}

const char* WebSocket::firmware_version_ = FIRMWARE_VERSION;

const char* WebSocket::request_id_key_ = "request_id";
const char* WebSocket::type_key_ = "type";

const char* WebSocket::telemetry_peripheral_key_ = "peripheral";
const char* WebSocket::fixed_peripheral_key_ = "fp_id";
const char* WebSocket::time_key_ = "time";

const char* WebSocket::uuid_key_ = "uuid";
const char* WebSocket::result_status_key_ = "status";
const char* WebSocket::result_detail_key_ = "detail";
const char* WebSocket::result_success_name_ = "success";
const char* WebSocket::result_fail_name_ = "fail";
const char* WebSocket::result_state_key_ = "state";

const char* WebSocket::limit_event_type_ = "lim";
const char* WebSocket::result_type_ = "result";
const char* WebSocket::telemetry_type_ = "tel";

const char* WebSocket::action_key_ = "action";
const char* WebSocket::behavior_key_ = "behav";
const char* WebSocket::task_key_ = "task";
const char* WebSocket::system_type_ = "sys";
const char* WebSocket::lac_key_ = "lac";

const char* WebSocket::default_core_domain_ = "core.inamata.io";
const char* WebSocket::default_ws_url_path_ = "/controller-ws/v1/";

const char* WebSocket::limit_id_key_ = "limit_id";
const char* WebSocket::fixed_peripheral_id_key_ = "fp_id";
const char* WebSocket::fixed_dpt_id_key_ = "fdpt_id";

}  // namespace inamata
