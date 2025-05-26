#pragma once

#include <TaskSchedulerDeclarations.h>
#include <WString.h>

#include "lac/lac_controller.h"
#include "managers/action_controller.h"
#include "managers/behavior_controller.h"
#include "managers/ota_updater.h"
#include "managers/service_getters.h"
#include "managers/web_socket.h"
#include "managers/wifi_network.h"
#include "peripheral/peripheral_controller.h"
#include "peripheral/peripheral_factory.h"
#include "tasks/task_controller.h"
#include "tasks/task_factory.h"
#include "tasks/task_removal_task.h"

namespace inamata {

/**
 * All the system services
 *
 * The services cover functions such as the connection to the server,
 * the peripheral and task controller as well as the scheduler. The task and
 * peripheral services are static instances, while the network and server are
 * created by the setup procedure setupNode().
 */
class Services {
 public:
  /**
   * Set the service getters for the static services (task and peripheral
   * controller as well as task removal task).
   */
  Services();
  virtual ~Services() = default;

  std::shared_ptr<WiFiNetwork> getWifiNetwork();
  void setWifiNetwork(std::shared_ptr<WiFiNetwork> wifi_network);
#ifdef GSM_NETWORK
  std::shared_ptr<GsmNetwork> getGsmNetwork();
  void setGsmNetwork(std::shared_ptr<GsmNetwork> gsm_network);
#endif
  std::shared_ptr<WebSocket> getWebSocket();
  void setWebSocket(std::shared_ptr<WebSocket> web_socket);

  std::shared_ptr<Storage> getStorage();
  void setStorage(std::shared_ptr<Storage> storage);

  std::shared_ptr<BleServer> getBleServer();
  void setBleServer(std::shared_ptr<BleServer> ble_server);
  std::shared_ptr<ConfigManager> getConfigManager();
  void setConfigManager(std::shared_ptr<ConfigManager> config_manager);

  std::shared_ptr<LoggingManager> getLoggingManager();
  void setLoggingManager(std::shared_ptr<LoggingManager> log_manager);

  static ActionController& getActionController();
  static BehaviorController& getBehaviorController();
  static peripheral::PeripheralController& getPeripheralController();
  static tasks::TaskController& getTaskController();
  static lac::LacController& getLacController();
  static OtaUpdater& getOtaUpdater();

  static Scheduler& getScheduler();

  /**
   * Get callbacks to get the pointers to dynamic services (network and server)
   *
   * \return Struct with callbacks to get pointers to the services
   */
  ServiceGetters getGetters();

 private:
  /// Handles WiFi network connectivity and time synchronization
  std::shared_ptr<WiFiNetwork> wifi_network_;
#ifdef GSM_NETWORK
  /// Handles GSM network connectivity
  std::shared_ptr<GsmNetwork> gsm_network_;
#endif
  /// Handles communication to the server
  std::shared_ptr<WebSocket> web_socket_;
  /// Handles FS / EEPROM storage
  std::shared_ptr<Storage> storage_;
  /// Handles Low-Energy Bluetooth
  std::shared_ptr<BleServer> ble_server_;
  /// Executes the active tasks

  std::shared_ptr<ConfigManager> config_manager_;
  std::shared_ptr<LoggingManager> log_manager_;
  static Scheduler scheduler_;

  /// Handles commands with controller actions
  static ActionController action_controller_;
  /// Handles commands for behavior
  static BehaviorController behavior_controller_;
  /// Creates peripherals with the registered peripheral factory callbacks
  static peripheral::PeripheralFactory peripheral_factory_;
  /// Handles server requests to create / delete peripherals
  static peripheral::PeripheralController peripheral_controller_;
  /// Creates tasks with the registered task factory callbacks
  static tasks::TaskFactory task_factory_;
  /// Handles server requests to start / stop tasks
  static tasks::TaskController task_controller_;
  /// Singleton to delete stopped tasks and inform the server
  static tasks::TaskRemovalTask task_removal_task_;
  /// Manages local action chains
  static lac::LacController lac_controller_;
  // /// Manages UI (buttons and LEDs)
  // static UiController ui_controller_;
  /// Singleton to perform OTA updates
  static OtaUpdater ota_updater_;
};

}  // namespace inamata
