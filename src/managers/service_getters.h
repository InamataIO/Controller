#pragma once
#include <functional>

#include "managers/ble_server.h"
#include "managers/config_menu.h"
#ifdef GSM_NETWORK
#include "managers/gsm_network.h"
#endif
#include "managers/log_manager.h"
#include "managers/storage.h"
#include "managers/web_socket.h"
#include "managers/wifi_network.h"

namespace inamata {
/**
 * Struct with callbacks to get dynamic system services
 */
struct ServiceGetters {
  ServiceGetters() = default;
  ServiceGetters(
      std::function<std::shared_ptr<WiFiNetwork>()> get_wifi_network,
#ifdef GSM_NETWORK
      std::function<std::shared_ptr<GsmNetwork>()> get_gsm_network,
#endif
      std::function<std::shared_ptr<WebSocket>()> get_web_socket,
      std::function<std::shared_ptr<Storage>()> get_storage,
      std::function<std::shared_ptr<BleServer>()> get_ble_server,
      std::function<std::shared_ptr<ConfigManager>()> get_config_manager,
      std::function<std::shared_ptr<LoggingManager>()> get_log_manager)
      : getWifiNetwork(get_wifi_network),
#ifdef GSM_NETWORK
        getGsmNetwork(get_gsm_network),
#endif
        getWebSocket(get_web_socket),
        getStorage(get_storage),
        getBleServer(get_ble_server),
        getConfigManager(get_config_manager),
        getLoggingManager(get_log_manager) {
  }

  std::function<std::shared_ptr<WiFiNetwork>()> getWifiNetwork = []() {
    return nullptr;
  };
#ifdef GSM_NETWORK
  std::function<std::shared_ptr<GsmNetwork>()> getGsmNetwork = []() {
    return nullptr;
  };
#endif
  std::function<std::shared_ptr<WebSocket>()> getWebSocket = []() {
    return nullptr;
  };
  std::function<std::shared_ptr<Storage>()> getStorage = []() {
    return nullptr;
  };
  std::function<std::shared_ptr<BleServer>()> getBleServer = []() {
    return nullptr;
  };
  std::function<std::shared_ptr<ConfigManager>()> getConfigManager = []() {
    return nullptr;
  };
  std::function<std::shared_ptr<LoggingManager>()> getLoggingManager = []() {
    return nullptr;
  };

  static const __FlashStringHelper* wifi_network_nullptr_error_;
#ifdef GSM_NETWORK
  static const __FlashStringHelper* gsm_network_nullptr_error_;
#endif
  static const __FlashStringHelper* web_socket_nullptr_error_;
  static const __FlashStringHelper* storage_nullptr_error_;
  static const __FlashStringHelper* ble_server_nullptr_error_;
  static const __FlashStringHelper* config_manager_nullptr_error_;
  static const __FlashStringHelper* log_manager_nullptr_error_;
};
}  // namespace inamata
