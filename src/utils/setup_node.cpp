#include "setup_node.h"

#include "managers/action_controller.h"
#include "managers/time_manager.h"
#include "managers/web_socket.h"
#include "peripheral/fixed.h"
#include "tasks/configman/configman_task.h"
#include "tasks/connectivity/connectivity.h"
#include "tasks/fixed/config.h"
#include "tasks/system_monitor/system_monitor.h"

namespace inamata {

bool loadNetwork(Services& services, JsonObjectConst secrets) {
  JsonArrayConst wifi_aps_doc = secrets[Storage::wifi_aps_key_];
  TRACEF("Found %u APs in secrets\r\n", wifi_aps_doc.size());

  std::vector<WiFiAP> wifi_aps;
  for (const JsonObjectConst i : wifi_aps_doc) {
    wifi_aps.emplace_back(i[Storage::wifi_ap_ssid_key_].as<const char*>(),
                          i[Storage::wifi_ap_password_key_].as<const char*>(),
                          -1, false);
  }
  String controller_name = secrets["name"].as<const char*>();
  if (controller_name.isEmpty()) {
    controller_name = Storage::device_type_name_;
  }
  services.setWifiNetwork(
      std::make_shared<WiFiNetwork>(wifi_aps, controller_name));

#ifdef DEVICE_TYPE_FIRE_DATA_LOGGER
  services.setGsmNetwork(std::make_shared<GsmNetwork>());
#endif
  return true;
}

bool loadWebsocket(Services& services, JsonObjectConst secrets) {
  using namespace std::placeholders;

  // Get the required data from the secrets file
  JsonVariantConst ws_token = secrets[Storage::ws_token_key_];
  JsonVariantConst core_domain = secrets[Storage::core_domain_key_];
  JsonVariantConst ws_url_path = secrets[Storage::ws_url_path_key_];
  JsonVariantConst secure_url = secrets[Storage::secure_url_key_];

  // Get the peripheral and task controllers
  ActionController& action_controller = services.getActionController();
  BehaviorController& behavior_controller = services.getBehaviorController();
  peripheral::PeripheralController& peripheral_controller =
      services.getPeripheralController();
  tasks::TaskController& task_controller = services.getTaskController();
  lac::LacController& lac_controller = services.getLacController();
  OtaUpdater& ota_updater = services.getOtaUpdater();

  // Create a websocket instance as the server interface
  WebSocket::Config config{
      .action_controller_callback =
          std::bind(&ActionController::handleCallback, &action_controller, _1),
      .behavior_controller_callback = std::bind(
          &BehaviorController::handleCallback, &behavior_controller, _1),
      .set_behavior_register_data = std::bind(
          &BehaviorController::setRegisterData, &behavior_controller, _1),
      .get_peripheral_ids =
          std::bind(&peripheral::PeripheralController::getPeripheralIDs,
                    &peripheral_controller),
      .peripheral_controller_callback =
          std::bind(&peripheral::PeripheralController::handleCallback,
                    &peripheral_controller, _1),
      .get_task_ids =
          std::bind(&tasks::TaskController::getTaskIDs, &task_controller),
      .task_controller_callback = std::bind(
          &tasks::TaskController::handleCallback, &task_controller, _1),
      .lac_controller_callback =
          std::bind(&lac::LacController::handleCallback, &lac_controller, _1),
      .ota_update_callback =
          std::bind(&OtaUpdater::handleCallback, &ota_updater, _1),
      .core_domain = core_domain.as<const char*>(),
      .ws_url_path = ws_url_path.as<const char*>(),
      .ws_token = ws_token.as<const char*>(),
      .secure_url = secure_url};
  services.setWebSocket(std::make_shared<WebSocket>(config));
  return true;
}

bool createSystemTasks(Services& services) {
  // Temporary storage for the created system tasks
  std::vector<tasks::BaseTask*> tasks;

  // Create the system tasks
  tasks.push_back(new tasks::connectivity::CheckConnectivity(
      services.getGetters(), services.getScheduler()));
  tasks.push_back(new tasks::system_monitor::SystemMonitor(
      services.getGetters(), services.getScheduler()));

  // Check if they were created, enable them and check if they started
  for (tasks::BaseTask* task : tasks) {
    if (!task) {
      TRACELN("Failed creating task");
      return false;
    }
    task->enable();
    if (!task->isValid()) {
      TRACELN(task->getError().toString());
      delete task;
      return false;
    }
  }
  return true;
}

bool loadLocalPeripherals(Services& services) {
  JsonDocument peripheral_doc;
  ErrorResult error = services.getStorage()->loadPeripherals(peripheral_doc);
  if (error.isError()) {
    services.getStorage()->deletePeripherals();
    // services.getStorage()->deleteTasks();
    // services.getStorage()->deleteLacs();

    // Return true avoids boot loop, but delete stored peris, tasks and LACs
    return true;
  }

  JsonArray peripherals = peripheral_doc.isNull()
                              ? peripheral_doc.to<JsonArray>()
                              : peripheral_doc.as<JsonArray>();
  for (JsonVariantConst peripheral : peripherals) {
    ErrorResult error = services.getPeripheralController().add(peripheral);
    TRACEF("Local peri: %s : %d\r\n",
           peripheral[peripheral::Peripheral::uuid_key_].as<const char*>(),
           error.isError());
    if (error.isError()) {
      // TODO: Save error and send to server
      Serial.println(error.toString());
      services.getStorage()->deletePeripherals();
      // services.getStorage()->deleteTasks();
      // services.getStorage()->deleteLacs();

      // Return false to reboot after stored peris, tasks and LACs are deleted
      return false;
    }
  }
  return true;
}

bool loadFixedPeripherals(Services& services) {
  JsonDocument peripherals_doc;
  for (auto config : peripheral::fixed::configs) {
    if (!config) {
      continue;
    }
    DeserializationError error = deserializeJson(peripherals_doc, config);
    if (error) {
      TRACEF("Fixed peri JSON fail: %s\r\n", error.c_str());
      return false;
    }
    for (auto peripheral : peripherals_doc.as<JsonArray>()) {
      ErrorResult error = services.getPeripheralController().add(peripheral);
      if (error.isError()) {
        TRACEF("Init fixed peri fail: %s\r\n", error.toString().c_str());
        TRACEJSON(peripheral);
        return false;
      }
    }
  }

  return true;
}

bool setupNode(Services& services) {
  // Enable serial communication and prints
  Serial.begin(115200);
  delay(2000);
  Serial.print("Fimware version: ");
  Serial.println(WebSocket::firmware_version_);

  // Load and start subsystems that need secrets
  services.setStorage(std::make_shared<Storage>());
  services.getStorage()->openFS();
  bool success;
  {
    JsonDocument secrets_doc;
    services.getStorage()->loadSecrets(secrets_doc);
    JsonObject secrets = secrets_doc.isNull() ? secrets_doc.to<JsonObject>()
                                              : secrets_doc.as<JsonObject>();
    success = loadNetwork(services, secrets);
    if (!success) {
      return false;
    }
    success = loadWebsocket(services, secrets);
    if (!success) {
      return false;
    }
  }

  // Create the BLE server
  services.setBleServer(std::make_shared<BleServer>());
  if (peripheral::fixed::configs[0] != nullptr) {
    success = loadFixedPeripherals(services);
  } else {
    success = loadLocalPeripherals(services);
  }
  if (!success) {
    return false;
  }

#ifdef RTC_MANAGER
  TimeManager::initRTC();
#endif

#ifdef CONFIGURATION_MANAGER
  services.setLoggingManager(std::make_shared<LoggingManager>());
  services.setConfigManager(std::make_shared<ConfigManager>());

  tasks::config_man::ConfigurationManagementTask* config_task =
      new tasks::config_man::ConfigurationManagementTask(
          services.getGetters(), services.getScheduler());

  if (!config_task->isValid()) {
    TRACELN(config_task->getError().toString());
    delete config_task;
  }
#endif

  if (peripheral::fixed::configs[0] != nullptr) {
    JsonDocument behavior_doc;
    services.getStorage()->loadBehavior(behavior_doc);
    JsonObjectConst behavior_config = behavior_doc.as<JsonObjectConst>();
    Services::getBehaviorController().handleConfig(behavior_config);
    tasks::fixed::startFixedTasks(services.getGetters(),
                                  services.getScheduler(), behavior_config);
  }

  success = createSystemTasks(services);
  if (!success) {
    return false;
  }

  return true;
}

}  // namespace inamata
