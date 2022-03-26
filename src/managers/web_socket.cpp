#include "web_socket.h"

#include <esp_tls.h>

namespace bernd_box {

WebSocket::WebSocket(const WebSocket::Config& config, String&& root_cas)
    : get_peripheral_ids_(config.get_peripheral_ids),
      peripheral_controller_callback_(config.peripheral_controller_callback),
      get_task_ids_(config.get_task_ids),
      task_controller_callback_(config.task_controller_callback),
      ota_update_callback_(config.ota_update_callback),
      core_domain_(config.core_domain),
      secure_url_(config.secure_url),
      root_cas_(root_cas) {
  ws_token_ = F("token_");
  ws_token_ += config.ws_token;
}

const String& WebSocket::type() {
  static const String name{"WebSocket"};
  return name;
}

bool WebSocket::isConnected() {
  const bool is_connected = WebSocketsClient::isConnected();
  updateUpDownTime(is_connected);
  return is_connected;
}

bool WebSocket::connect(std::chrono::seconds timeout) {
  // Configure the WebSocket interface with the server, TLS certificate and the
  // reconnect interval
  if (!is_setup_) {
    is_setup_ = true;
    if (secure_url_) {
      beginSslWithCA(core_domain_.c_str(), 443, controller_path_,
                     root_cas_.c_str(), ws_token_.c_str());
    } else {
      begin(core_domain_.c_str(), 8000, controller_path_, ws_token_.c_str());
    }
    onEvent(std::bind(&WebSocket::handleEvent, this, _1, _2, _3));
    setReconnectInterval(5000);
  }

  if (!isConnected()) {
    // Attempt to connect to the server until connected or it times out
    const auto connect_start = std::chrono::steady_clock::now();
    while (!isConnected()) {
      if (std::chrono::steady_clock::now() - connect_start > timeout) {
        return false;
      }
      loop();
    }
    sendUpDownTimeData();
  }

  return true;
}

void WebSocket::handle() { loop(); }

void WebSocket::send(const String& name, double value) {
  restartOnUnimplementedFunction();
}
void WebSocket::send(const String& name, int value) {
  restartOnUnimplementedFunction();
}
void WebSocket::send(const String& name, bool value) {
  restartOnUnimplementedFunction();
}

void WebSocket::send(const String& name, DynamicJsonDocument& doc) {
  doc["type"] = "tel";
  doc["name"] = name;

  // Calculate the size of the resultant serialized JSON, create a buffer of
  // that size and serialize the JSON into that buffer.
  // Add extra byte for the null terminator
  std::vector<char> register_buf = std::vector<char>(measureJson(doc) + 1);
  size_t n = serializeJson(doc, register_buf.data(), register_buf.size());

  sendTXT(register_buf.data(), n);
}

void WebSocket::send(const String& name, const char* value, size_t length) {
  restartOnUnimplementedFunction();
}

void WebSocket::sendTelemetry(const utils::UUID& task_id, JsonObject data) {
  data[Server::type_key_] = Server::telemetry_type_;
  data[Server::task_id_key_] = task_id.toString();

  std::vector<char> register_buf = std::vector<char>(measureJson(data) + 1);
  size_t n = serializeJson(data, register_buf.data(), register_buf.size());
  Serial.printf("Telemetry size: %i\n", n);

  sendTXT(register_buf.data(), n);
}

void WebSocket::sendRegister() {
  DynamicJsonDocument doc(BB_JSON_PAYLOAD_SIZE);

  // Use the register message type
  doc["type"] = "reg";

  // Set the firmware version number
  doc["version"] = firmware_version_;

  // Collect all added peripheral ids and write them to a JSON doc
  std::vector<utils::UUID> peripheral_ids = get_peripheral_ids_();
  if (!peripheral_ids.empty()) {
    JsonArray peripherals = doc.createNestedArray(F("peripherals"));
    for (const auto& peripheral_id : peripheral_ids) {
      peripherals.add(peripheral_id.toString());
    }
  }

  // Collect all running task ids and write them to a JSON doc
  std::vector<utils::UUID> task_ids = get_task_ids_();
  if (!task_ids.empty()) {
    JsonArray tasks = doc.createNestedArray(F("tasks"));
    for (const auto& task_id : task_ids) {
      if (task_id.isValid()) {
        tasks.add(task_id.toString());
      }
    }
  }

  // Calculate the size of the resultant serialized JSON, create a buffer of
  // that size and serialize the JSON into that buffer.
  // Add extra byte for the null terminator
  std::vector<char> register_buf = std::vector<char>(measureJson(doc) + 1);
  size_t n = serializeJson(doc, register_buf.data(), register_buf.size());

  sendTXT(register_buf.data(), n);
}

void WebSocket::sendError(const String& who, const String& message) {
  Serial.println(message);

  DynamicJsonDocument doc(BB_JSON_PAYLOAD_SIZE);

  // Use ther error message type
  doc["type"] = "err";

  // Place the error message
  doc["message"] = message.c_str();

  std::vector<char> register_buf = std::vector<char>(measureJson(doc) + 1);
  size_t n = serializeJson(doc, register_buf.data(), register_buf.size());

  sendTXT(register_buf.data(), n);
}

void WebSocket::sendError(const ErrorResult& error, const String& request_id) {
  Serial.printf("origin: %s message: %s request_id: %s\n", error.who_.c_str(),
                error.detail_.c_str(), request_id.c_str());

  DynamicJsonDocument doc(BB_JSON_PAYLOAD_SIZE);

  // Use ther error message type
  doc["type"] = "err";

  // Where the message originated from
  doc["context"] = error.who_.c_str();

  // The error itself
  doc["message"] = error.detail_.c_str();

  // The request ID to enable tracing
  doc["request_id"] = request_id.c_str();

  std::vector<char> register_buf = std::vector<char>(measureJson(doc) + 1);
  size_t n = serializeJson(doc, register_buf.data(), register_buf.size());

  sendTXT(register_buf.data(), n);
}

void WebSocket::sendResults(JsonObjectConst results) {
  std::vector<char> buffer = std::vector<char>(measureJson(results) + 1);
  size_t n = serializeJson(results, buffer.data(), buffer.size());

  sendTXT(buffer.data(), n);
}

void WebSocket::sendSystem(JsonObject data) {
  data[Server::type_key_] = Server::system_type_;

  std::vector<char> data_buf = std::vector<char>(measureJson(data) + 1);
  size_t n = serializeJson(data, data_buf.data(), data_buf.size());

  sendTXT(data_buf.data(), n);
}

const String& WebSocket::getRootCas() const { return root_cas_; }

void WebSocket::handleEvent(WStype_t type, uint8_t* payload, size_t length) {
  // Print class type before the printing the message type
  Serial.print(this->type());
  switch (type) {
    case WStype_DISCONNECTED: {
      Serial.println(F(": Disconnected!"));
    } break;
    case WStype_CONNECTED: {
      Serial.print(F("Connected to: "));
      Serial.println(reinterpret_cast<char*>(payload));
    } break;
    case WStype_TEXT: {
      Serial.print(F("Got text: "));
      Serial.println(reinterpret_cast<char*>(payload));
      handleData(payload, length);
    } break;
    case WStype_PING:
      Serial.println(F("Received ping"));
      break;
    case WStype_BIN:
    case WStype_ERROR:
    case WStype_FRAGMENT_TEXT_START:
    case WStype_FRAGMENT_BIN_START:
    case WStype_FRAGMENT:
    case WStype_FRAGMENT_FIN:
    case WStype_PONG:
      Serial.print(F("Unhandled message type: "));
      Serial.println(type);
      break;
  }
}

void WebSocket::handleData(const uint8_t* payload, size_t length) {
  // Deserialize the JSON object into allocated memory
  DynamicJsonDocument doc(BB_JSON_PAYLOAD_SIZE);
  const DeserializationError error = deserializeJson(doc, payload, length);
  if (error) {
    sendError(type(), String(F("Deserialize failed: ")) + error.c_str());
    return;
  }

  // Pass the message to the peripheral and task handlers
  peripheral_controller_callback_(doc.as<JsonObjectConst>());
  task_controller_callback_(doc.as<JsonObjectConst>());
  ota_update_callback_(doc.as<JsonObjectConst>());
}

void WebSocket::updateUpDownTime(const bool is_connected) {
  if (is_connected != was_connected_) {
    was_connected_ = is_connected;
    const auto now = std::chrono::steady_clock::now();
    if (is_connected) {
      // Connection went up
      if (last_connect_down_ != std::chrono::steady_clock::time_point::min()) {
        // Only update if last_connect_down_ has been set
        last_down_duration_ = now - last_connect_down_;
      }
      last_connect_up_ = now;
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
    DynamicJsonDocument doc(BB_JSON_PAYLOAD_SIZE);
    if (last_up_duration_ != std::chrono::steady_clock::duration::min()) {
      // If the up duration is valid, print and send the data
      int64_t last_up_duration_s =
          std::chrono::duration_cast<std::chrono::seconds>(last_up_duration_)
              .count();
      Serial.printf("Last WS up duration: %llds\n", last_up_duration_s);
      doc[F("last_ws_up_duration_s")] =
          std::chrono::duration_cast<std::chrono::seconds>(last_up_duration_)
              .count();
    }
    if (last_down_duration_ != std::chrono::steady_clock::duration::min()) {
      // If the down duration is valid, print and send the data
      int64_t last_down_duration_s =
          std::chrono::duration_cast<std::chrono::seconds>(last_down_duration_)
              .count();
      Serial.printf("Last WS down duration: %llds\n", last_down_duration_s);
      doc[F("last_ws_down_duration_s")] =
          std::chrono::duration_cast<std::chrono::seconds>(last_down_duration_)
              .count();
    }
    sendSystem(doc.as<JsonObject>());
  }
}

void WebSocket::restartOnUnimplementedFunction() {
  Serial.print(type());
  Serial.print(F(": Unimplemented Function"));
  delay(10000);
  abort();
}

const __FlashStringHelper* WebSocket::firmware_version_ = F(FIRMWARE_VERSION);

}  // namespace bernd_box
