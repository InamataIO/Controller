#include "services.h"

#include "configuration.h"

namespace inamata {

Services::Services() {
  ServiceGetters getters = getGetters();

  action_controller_.setServices(getters);
  behavior_controller_.setServices(getters);
  peripheral_controller_.setServices(getters);
  task_controller_.setServices(getters);
  task_removal_task_.setServices(getters);
  lac_controller_.setServices(getters);
  ota_updater_.setServices(getters);
}

std::shared_ptr<WiFiNetwork> Services::getWifiNetwork() {
  return wifi_network_;
}

void Services::setWifiNetwork(std::shared_ptr<WiFiNetwork> wifi_network) {
  wifi_network_ = wifi_network;
}
#ifdef GSM_NETWORK
std::shared_ptr<GsmNetwork> Services::getGsmNetwork() { return gsm_network_; }

void Services::setGsmNetwork(std::shared_ptr<GsmNetwork> gsm_network) {
  gsm_network_ = gsm_network;
}
#endif
std::shared_ptr<WebSocket> Services::getWebSocket() { return web_socket_; }

void Services::setWebSocket(std::shared_ptr<WebSocket> web_socket) {
  web_socket_ = web_socket;
}

std::shared_ptr<Storage> Services::getStorage() { return storage_; }

void Services::setStorage(std::shared_ptr<Storage> storage) {
  storage_ = storage;
}

std::shared_ptr<BleServer> Services::getBleServer() { return ble_server_; }

void Services::setBleServer(std::shared_ptr<BleServer> ble_server) {
  ble_server_ = ble_server;
}

std::shared_ptr<ConfigManager> Services::getConfigManager() {
  return config_manager_;
}

void Services::setConfigManager(std::shared_ptr<ConfigManager> config_manager) {
  config_manager_ = config_manager;
}

std::shared_ptr<LoggingManager> Services::getLoggingManager() {
  return log_manager_;
}

void Services::setLoggingManager(std::shared_ptr<LoggingManager> log_manager) {
  log_manager_ = log_manager;
}

ActionController& Services::getActionController() { return action_controller_; }

BehaviorController& Services::getBehaviorController() {
  return behavior_controller_;
}

peripheral::PeripheralController& Services::getPeripheralController() {
  return peripheral_controller_;
}

tasks::TaskController& Services::getTaskController() {
  return task_controller_;
}

lac::LacController& Services::getLacController() { return lac_controller_; }

OtaUpdater& Services::getOtaUpdater() { return ota_updater_; }

Scheduler& Services::getScheduler() { return scheduler_; }

ServiceGetters Services::getGetters() {
  ServiceGetters getters(std::bind(&Services::getWifiNetwork, this),
#ifdef GSM_NETWORK
                         std::bind(&Services::getGsmNetwork, this),
#endif
                         std::bind(&Services::getWebSocket, this),
                         std::bind(&Services::getStorage, this),
                         std::bind(&Services::getBleServer, this),
                         std::bind(&Services::getConfigManager, this),
                         std::bind(&Services::getLoggingManager, this));
  return getters;
}

Scheduler Services::scheduler_{};

ActionController Services::action_controller_{};

BehaviorController Services::behavior_controller_{};

peripheral::PeripheralFactory Services::peripheral_factory_{};

peripheral::PeripheralController Services::peripheral_controller_{
    peripheral_factory_};

tasks::TaskFactory Services::task_factory_{scheduler_};

tasks::TaskController Services::task_controller_{scheduler_, task_factory_};

tasks::TaskRemovalTask Services::task_removal_task_{scheduler_};

lac::LacController Services::lac_controller_{scheduler_};

OtaUpdater Services::ota_updater_{scheduler_};

// UiController Services::ui_controller_{scheduler_};

}  // namespace inamata
