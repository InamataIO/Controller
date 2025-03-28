#pragma once
#include <functional>

#include "managers/ble_server.h"
#include "managers/network.h"
#include "managers/storage.h"
#include "managers/web_socket.h"
#include "managers/config_menu.h"

namespace inamata {
/**
 * Struct with callbacks to get dynamic system services
 */
struct ServiceGetters {
  ServiceGetters() = default;
  ServiceGetters(std::function<std::shared_ptr<Network>()> get_network,
                 std::function<std::shared_ptr<WebSocket>()> get_web_socket,
                 std::function<std::shared_ptr<Storage>()> get_storage,
                 std::function<std::shared_ptr<BleServer>()> get_ble_server,
                 std::function<std::shared_ptr<ConfigManager>()> get_config_manager)
      : getNetwork(get_network),
        getWebSocket(get_web_socket),
        getStorage(get_storage),
        getBleServer(get_ble_server),
        getConfigManager(get_config_manager) {}

  std::function<std::shared_ptr<Network>()> getNetwork = []() {
    return nullptr;
  };
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

  static const __FlashStringHelper* network_nullptr_error_;
  static const __FlashStringHelper* web_socket_nullptr_error_;
  static const __FlashStringHelper* storage_nullptr_error_;
  static const __FlashStringHelper* ble_server_nullptr_error_;
  static const __FlashStringHelper* config_manager_nullptr_error_;
};
}  // namespace inamata
