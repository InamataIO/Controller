#include "setup_node.h"

#include "managers/action_controller.h"
#include "managers/web_socket.h"
#include "tasks/connectivity/connectivity.h"
#include "tasks/system_monitor/system_monitor.h"

namespace inamata {

bool loadNetwork(Services& services, JsonObjectConst secrets) {
  JsonArrayConst wifi_aps_doc = secrets[Storage::wifi_aps_key_];
  TRACEF("Found %u APs in secrets\n", wifi_aps_doc.size());

  std::vector<WiFiAP> wifi_aps;
  for (const JsonObjectConst i : wifi_aps_doc) {
    wifi_aps.emplace_back(i[Storage::wifi_ap_ssid_key_].as<const char*>(),
                          i[Storage::wifi_ap_password_key_].as<const char*>(),
                          -1, false);
  }
  String controller_name = secrets[F("name")].as<const char*>();
  if (controller_name.isEmpty()) {
    controller_name = Storage::device_type_name_;
  }
  services.setNetwork(std::make_shared<Network>(wifi_aps, controller_name));
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
  peripheral::PeripheralController& peripheral_controller =
      services.getPeripheralController();
  tasks::TaskController& task_controller = services.getTaskController();
  lac::LacController& lac_controller = services.getLacController();
#ifdef ESP32
  OtaUpdater& ota_updater = services.getOtaUpdater();
#endif

  // Create a websocket instance as the server interface
  WebSocket::Config config{
      .action_controller_callback =
          std::bind(&ActionController::handleCallback, &action_controller, _1),
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
#ifdef ESP32
      .ota_update_callback =
          std::bind(&OtaUpdater::handleCallback, &ota_updater, _1),
#endif
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
      TRACELN(F("Failed creating task"));
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

  for (auto peripheral : peripheral_doc.as<JsonArray>()) {
    ErrorResult error = services.getPeripheralController().add(peripheral);
    TRACEF("Loaded peri: %s\n",
           peripheral[peripheral::Peripheral::uuid_key_].as<const char*>());
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

bool setupNode(Services& services) {
#ifdef ATHOM_PLUG_V2
  // TODO: Place in setup function or create default boot pin config
  const uint8_t relay_pin = 12;
  pinMode(relay_pin, OUTPUT);
  digitalWrite(relay_pin, HIGH);
#endif

  // Enable serial communication and prints
  Serial.begin(115200);
  Serial.print(F("Fimware version: "));
  Serial.println(WebSocket::firmware_version_);

  // Load and start subsystems that need secrets
  services.setStorage(std::make_shared<Storage>());
  services.getStorage()->openFS();
  bool success;
  {
    JsonDocument secrets_doc;
    services.getStorage()->loadSecrets(secrets_doc);
    success = loadNetwork(services, secrets_doc.as<JsonObjectConst>());
    if (!success) {
      return false;
    }
    success = loadWebsocket(services, secrets_doc.as<JsonObjectConst>());
    if (!success) {
      return false;
    }
  }

  // Create the BLE server
  services.setBleServer(std::make_shared<BleServer>());

  success = createSystemTasks(services);
  if (!success) {
    return false;
  }

  success = loadLocalPeripherals(services);
  if (!success) {
    return false;
  }

  return true;
}

}  // namespace inamata
