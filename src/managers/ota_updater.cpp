#include "ota_updater.h"

#include <ArduinoJson.h>
#include <Update.h>
#include <yuarel.h>

#include <chrono>
#include <memory>

#ifdef GSM_NETWORK
#include "managers/network_client_impl.h"
#endif

#define OTA_TINY_GSM_MUX 1

namespace inamata {

static inline const char* url_path_and_query(const char* url) {
  const char* p = strstr(url, "://");
  p = p ? p + 3 : url;    // skip scheme if present
  return strchr(p, '/');  // returns NULL if no path
}

OtaUpdater::OtaUpdater(Scheduler& scheduler)
    : BaseTask(scheduler, Input(nullptr, true)) {}

const String& OtaUpdater::getType() const { return type(); }

const String& OtaUpdater::type() {
  static const String name{"OtaUpdater"};
  return name;
}

void OtaUpdater::setServices(ServiceGetters services) { services_ = services; }

void OtaUpdater::useNetwork(Network network) { network_ = network; }

void OtaUpdater::handleCallback(const JsonObjectConst& message) {
  JsonVariantConst update_command = message[update_command_key_];
  if (!update_command) {
    return;
  }

  // Abort if an update is already running and use the new request ID
  const char* request_id =
      message[WebSocket::request_id_key_].as<const char*>();
  if (is_updating_) {
    sendResult(status_fail_, update_in_progress_error_, request_id);
    return;
  }
  request_id_ = request_id;

  JsonVariantConst url = update_command[url_key_];
  if (!url.is<const char*>()) {
    sendResult(status_fail_, ErrorStore::genMissingProperty(
                                 url_key_, ErrorStore::KeyType::kString));
    return;
  }
  const char* url_str = url.as<const char*>();

  JsonVariantConst image_size = update_command[image_size_key_];
  if (!image_size.is<size_t>()) {
    sendResult(status_fail_,
               ErrorStore::genMissingProperty(image_size_key_,
                                              ErrorStore::KeyType::kString));
    return;
  }
  image_size_ = image_size;

  JsonVariantConst md5_hash = update_command[md5_hash_key_];
  if (!md5_hash.is<const char*>()) {
    sendResult(status_fail_, ErrorStore::genMissingProperty(
                                 md5_hash_key_, ErrorStore::KeyType::kString));
    return;
  }

  JsonVariantConst restart = update_command[restart_key_];
  if (!restart.is<bool>()) {
    sendResult(status_fail_, ErrorStore::genMissingProperty(
                                 restart_key_, ErrorStore::KeyType::kBool));
    return;
  }
  restart_ = restart;

  struct yuarel url_parts;
  std::vector<char> url_data(strlen(url_str) + 1, '\0');
  strlcpy(url_data.data(), url_str, url_data.size());
  if (yuarel_parse(&url_parts, &url_data[0]) == -1) {
    sendResult(status_fail_, String("Invalid OTA URL: ") + url_str);
    return;
  }
  const bool secure = (strcasecmp(url_parts.scheme, "https") == 0) &&
                      (network_ == Network::kWifi);
  const uint16_t port = secure ? 443 : 80;

  // Create WiFi or GSM client
  if (network_ == Network::kWifi) {
    wifi_client_ = std::make_unique<WiFiClientSecure>();
    wifi_http_client_ = std::make_unique<HTTPClient>();
  } else {
#ifdef GSM_NETWORK
    auto gsm_network = services_.getGsmNetwork();
    if (gsm_network) {
      gsm_https_client_ = std::make_unique<GsmHttpsClient>(
          gsm_network->modem_, url_parts.host, port, OTA_TINY_GSM_MUX, secure);
    }
#endif
  }

  if (!wifi_client_ && !wifi_http_client_
#ifdef GSM_NETWORK
      && !gsm_https_client_
#endif
  ) {
    sendResult(status_fail_, "Create client fail");
    return;
  }

  // Check if to use TLS or not
  bool connected = false;
  if (secure) {
    // Get and set the CA if available
    std::shared_ptr<WebSocket> web_socket = services_.getWebSocket();
    if (web_socket == nullptr) {
      TRACELN(ErrorResult(type(), ServiceGetters::web_socket_nullptr_error_)
                  .toString());
      clearClients();
      return;
    }
    if (network_ == Network::kWifi) {
      wifi_client_->setCACertBundle(rootca_crt_bundle_start,
                                    rootca_crt_bundle_len());
      connected = wifi_http_client_->begin(*wifi_client_, url_str);
    }
    // else if (network_ == Network::kGsm) {
    //   gsm_https_client_->ssl_client_->setCACertBundle(rootca_crt_bundle_start);
    //   gsm_https_client_->ssl_client_->setTimeout(60000);
    //   connected =
    //       gsm_https_client_->http_client_->connect(url_parts.host, port);
    // }
  } else {
    // Use an unencrypted connection
    if (network_ == Network::kWifi) {
      connected = wifi_http_client_->begin(url_str);
    } else if (network_ == Network::kGsm) {
#ifdef GSM_NETWORK
      connected =
          gsm_https_client_->http_client_->connect(url_parts.host, port);
#endif
    }
  }
  if (!connected) {
    sendResult(status_fail_, failed_to_connect_error_);
    clearClients();
    return;
  }

  // Start the HTTP GET request
  if (network_ == Network::kWifi) {
    const int status_code = wifi_http_client_->GET();
    if (status_code != HTTP_CODE_OK) {
      sendResult(status_fail_,
                 ErrorResult(type(), String(http_code_error_) + status_code)
                     .toString());
      clearClients();
      return;
    }
  } else {
#ifdef GSM_NETWORK
    const char* path_and_query = url_path_and_query(url_str);
    if (path_and_query == nullptr) {
      sendResult(status_fail_,
                 String("Invalid OTA URL (missing path): ") + url_str);
      clearClients();
      return;
    }
    const int error = gsm_https_client_->http_client_->get(path_and_query);
    if (error != 0) {
      sendResult(
          status_fail_,
          ErrorResult(type(), String(http_connect_error_) + error).toString());
      clearClients();
      return;
    }
    const int http_code = gsm_https_client_->http_client_->responseStatusCode();
    if (http_code < 200 || http_code >= 300) {
      sendResult(
          status_fail_,
          ErrorResult(type(), String(http_code_error_) + http_code).toString());
      clearClients();
      return;
    }
    const int body_bytes = gsm_https_client_->http_client_->contentLength();
    if (body_bytes >= 0) {
      if (body_bytes != image_size_) {
        sendResult(status_fail_,
                   ErrorResult(type(), String(size_mismatch_) + body_bytes +
                                           ':' + image_size_)
                       .toString());
        clearClients();
        return;
      }
    } else {
      TRACELN("Content length < 0");
    }
#endif
  }

  // Initialize the firmware update driver
  Update.setMD5(md5_hash.as<const char*>());
  Update.begin(image_size_);

  // Confirm to the server that the update process has started
  sendResult(status_start_, "");

  setIterations(-1);
  enable();
}

bool OtaUpdater::OnTaskEnable() {
  buffer_.resize(4096);
  is_updating_ = true;
  Task::delay(std::chrono::milliseconds(50).count());
  return true;
}

bool OtaUpdater::TaskCallback() {
  int bytes_read = 0;
  if (network_ == Network::kWifi) {
    // TODO: Crash if changing to WiFi mode during update
    WiFiClient* stream = wifi_http_client_->getStreamPtr();
    if (stream == nullptr) {
      sendResult(status_fail_, connection_lost_error_);
      clearClients();
      return false;
    }

    if (stream->available()) {
      bytes_read = stream->readBytes(buffer_.data(), buffer_.size());
      Update.write(buffer_.data(), bytes_read);
    }
  } else {
#ifdef GSM_NETWORK
    if (gsm_https_client_->http_client_->available()) {
      bytes_read = gsm_https_client_->http_client_->readBytes(buffer_.data(),
                                                              buffer_.size());
      Update.write(buffer_.data(), bytes_read);
    } else {
      const int percent = float(Update.progress()) / float(image_size_) * 100;
      TRACEF("Completed: %d, connected: %d, percent: %d\r\n",
             gsm_https_client_->http_client_->completed(),
             gsm_https_client_->http_client_->connected(), percent);
    }
#endif
  }

  // Inform the server of progress
  if (bytes_read) {
    const int percent = float(Update.progress()) / float(image_size_) * 100;
    if (last_percent_update < 0 || last_percent_update + 1 <= percent) {
      last_percent_update = percent;
      String detail = F("{\"done\":\"");
      detail += last_percent_update;
      detail += F("%\"}");
      sendResult(status_updating_, detail);
    }
  }

  // TODO: Check and print error in Update class

  // Check if the update is finished, successfully or errored out
  if (Update.isFinished()) {
    // Close the HTTP connection
    if (network_ == Network::kWifi) {
      wifi_http_client_->end();
    } else {
#ifdef GSM_NETWORK
      gsm_https_client_->http_client_->stop();
#endif
    }

    // Mark the update as complete and check if it succeeded
    const bool success = Update.end();
    if (success) {
      // If successful, inform the server and reboot
      sendResult(status_finish_, "");
    } else {
      // If not, inform the server and remove the updating lock
      sendResult(status_fail_, Update.errorString());
      restart_ = false;
    }
    disable();
  }

  Task::delay(std::chrono::milliseconds(50).count());
  return true;
}

void OtaUpdater::OnTaskDisable() {
  clearClients();

  if (restart_) {
    // Sleep to allow remaining packets to be sent
    ::delay(1500);
    esp_restart();
  }
  buffer_.clear();
  buffer_.shrink_to_fit();

  request_id_.clear();
  last_percent_update = -1;
  is_updating_ = false;
}

void OtaUpdater::sendResult(const char* status, const String& detail,
                            const char* request_id) {
  // Get the server instance to send the error message
  std::shared_ptr<WebSocket> web_socket = services_.getWebSocket();
  if (web_socket == nullptr) {
    TRACELN(ErrorResult(type(), ServiceGetters::web_socket_nullptr_error_)
                .toString());
    return;
  }
  // Create the error message
  JsonDocument doc_out;
  doc_out[WebSocket::type_key_] = WebSocket::result_type_;
  // Favor request ID from parameters over class member. Ignore if none given
  if (request_id != nullptr) {
    doc_out[WebSocket::request_id_key_] = request_id;
  } else if (!request_id_.isEmpty()) {
    doc_out[WebSocket::request_id_key_] = request_id_.c_str();
  }

  JsonObject update_result = doc_out[update_command_key_].to<JsonObject>();
  update_result[status_key_] = status;
  if (!detail.isEmpty()) {
    update_result[detail_key_] = detail.c_str();
  }
  // Send the error and clear the request ID to end trace line
  web_socket->sendResults(doc_out.as<JsonObject>());
}

void OtaUpdater::clearClients() {
  Update.abort();

  if (wifi_http_client_) {
    wifi_http_client_->end();
  }
  wifi_client_ = nullptr;
  wifi_http_client_ = nullptr;
#ifdef GSM_NETWORK
  if (gsm_https_client_) {
    gsm_https_client_->http_client_->stop();
  }
  gsm_https_client_ = nullptr;
#endif
}

const char* OtaUpdater::update_command_key_ = "update";
const char* OtaUpdater::url_key_ = "url";
const char* OtaUpdater::image_size_key_ = "size";
const char* OtaUpdater::md5_hash_key_ = "md5";
const char* OtaUpdater::restart_key_ = "restart";

const char* OtaUpdater::status_key_ = "status";
const char* OtaUpdater::status_start_ = "start";
const char* OtaUpdater::status_updating_ = "updating";
const char* OtaUpdater::status_finish_ = "finish";
const char* OtaUpdater::status_fail_ = "fail";
const char* OtaUpdater::detail_key_ = "detail";
const char* OtaUpdater::failed_to_connect_error_ = "Failed to connect";
const char* OtaUpdater::connection_lost_error_ = "Connection lost";
const char* OtaUpdater::update_in_progress_error_ =
    "OTA update already running";
const char* OtaUpdater::http_code_error_ = "HTTP code: ";
const char* OtaUpdater::http_connect_error_ = "Connect error: ";
const char* OtaUpdater::size_mismatch_ = "Size mismatch: ";

}  // namespace inamata
