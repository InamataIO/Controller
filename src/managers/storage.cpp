#include "storage.h"

#include "peripheral/peripheral.h"

namespace inamata {

#if defined(ENABLE_TRACE)
void listDir(fs::FS& fs, const char* dirname, uint8_t levels) {
  Serial.printf("Listing directory: %s\r\n", dirname);

  fs::File root = fs.open(dirname, "r+");
  if (!root) {
    Serial.println("- failed to open directory");
    return;
  }
  if (!root.isDirectory()) {
    Serial.println(" - not a directory");
    return;
  }

  fs::File file = root.openNextFile();
  while (file) {
    if (file.isDirectory()) {
      Serial.print("  DIR : ");

      Serial.print(file.name());
      time_t t = file.getLastWrite();
      struct tm* tmstruct = localtime(&t);
      Serial.printf("  LAST WRITE: %d-%02d-%02d %02d:%02d:%02d\n",
                    (tmstruct->tm_year) + 1900, (tmstruct->tm_mon) + 1,
                    tmstruct->tm_mday, tmstruct->tm_hour, tmstruct->tm_min,
                    tmstruct->tm_sec);

      if (levels) {
        listDir(fs, file.path(), levels - 1);
      }
    } else {
      Serial.print("  FILE: ");
      Serial.print(file.name());
      Serial.print("  SIZE: ");

      Serial.print(file.size());
      time_t t = file.getLastWrite();
      struct tm* tmstruct = localtime(&t);
      Serial.printf("  LAST WRITE: %d-%02d-%02d %02d:%02d:%02d\n",
                    (tmstruct->tm_year) + 1900, (tmstruct->tm_mon) + 1,
                    tmstruct->tm_mday, tmstruct->tm_hour, tmstruct->tm_min,
                    tmstruct->tm_sec);
    }
    file = root.openNextFile();
  }
}
#endif

void Storage::openFS() {
  // Initialize the file system
  if (!LittleFS.begin()) {
    if (!LittleFS.begin(true)) {
      TRACELN(F("Failed mounting LittleFS"));
    } else {
      TRACELN(F("Formatted LittleFS on fail"));
    }
  }
#ifdef ENABLE_TRACE
  listDir(LittleFS, "/", 1);
#endif
}

void Storage::closeFS() { LittleFS.end(); }

ErrorResult Storage::loadSecrets(JsonDocument& secrets_doc) {
  fs::File secrets_file = LittleFS.open(secrets_path_, "r");
  if (secrets_file) {
    DeserializationError error = deserializeJson(secrets_doc, secrets_file);
    secrets_file.close();
    if (error) {
      TRACEF("Failed parsing secrets.json: %s\n", error.c_str());
      return ErrorResult(type_, error.c_str());
    }
  } else {
    TRACELN(F("Failed opening (r) secrets.json"));
  }
  return ErrorResult();
}

void Storage::recursiveRm(const char* path) {
  if (strlen(path) == 0) {
    return;
  }
  fs::File root = LittleFS.open(path, "r");
  if (!root) {
    TRACEF("Can't open: %s\n", path);
    return;
  }
  if (!root.isDirectory()) {
    root.close();
    LittleFS.remove(path);
  } else {
    for (fs::File file = root.openNextFile(); file;
         file = root.openNextFile()) {
      if (!file) {
        TRACEF("Skipping: %s\n", file.path());
        continue;
      }
      if (file.isDirectory()) {
        recursiveRm(file.path());
      } else {
        String file_path = file.path();
        TRACEF("Delete %s\n", file_path.c_str());
        file.close();
        LittleFS.remove(file_path);
      }
    }
    LittleFS.rmdir(path);
  }
}

ErrorResult Storage::storeSecrets(JsonVariantConst secrets) {
  fs::File secrets_file = LittleFS.open(secrets_path_, "w+", true);
  if (!secrets_file) {
    return ErrorResult(type_, F("Failed opening (w+) secrets.json"));
  }

  size_t bytes_written = serializeJson(secrets, secrets_file);
  TRACEF("Saved secrets: %d : %d\n", bytes_written, measureJson(secrets));
  if (bytes_written == 0 && secrets.size()) {
    return ErrorResult(type_, F("Failed to write secrets"));
  }
  secrets_file.close();
#ifdef ENABLE_TRACE
  serializeJson(secrets, Serial);
#endif
  return ErrorResult();
}

ErrorResult Storage::saveWiFiAP(const WiFiAP& wifi_ap) {
  JsonDocument doc;
  ErrorResult error = loadSecrets(doc);
  if (error.isError()) {
    return error;
  }

  // Save WiFi credentials to list of known APs
  JsonVariant wifi_aps_value = doc[wifi_aps_key_];
  JsonArray wifi_aps;
  if (wifi_aps_value.isNull()) {
    wifi_aps = doc[wifi_aps_key_].to<JsonArray>();
  } else {
    wifi_aps = wifi_aps_value.as<JsonArray>();
  }

  // Check if AP SSID already exists
  JsonObject wifi_ap_obj;
  for (JsonObject known_wifi_ap : wifi_aps) {
    if (known_wifi_ap[wifi_ap_ssid_key_] == wifi_ap.ssid) {
      wifi_ap_obj = known_wifi_ap;
      break;
    }
  }
  if (wifi_ap_obj.isNull()) {
    wifi_ap_obj = wifi_aps.add<JsonObject>();
  }

  wifi_ap_obj[wifi_ap_ssid_key_] = wifi_ap.ssid;
  wifi_ap_obj[wifi_ap_password_key_] = wifi_ap.password;

  error = storeSecrets(doc);
  return error;
}

ErrorResult Storage::saveWsUrl(const char* domain, const char* path,
                               bool secure_url) {
  JsonDocument doc;
  ErrorResult error = loadSecrets(doc);
  if (error.isError()) {
    return error;
  }

  if (domain && strlen(path) >= 1) {
    doc[core_domain_key_] = domain;
  } else {
    doc.remove(core_domain_key_);
  }
  if (path && strlen(path) >= 1) {
    doc[ws_url_path_key_] = path;
  } else {
    doc.remove(ws_url_path_key_);
  }
  doc[secure_url_key_] = secure_url;

  error = storeSecrets(doc);
  return error;
}

ErrorResult Storage::deleteWsUrl() {
  JsonDocument doc;
  ErrorResult error = loadSecrets(doc);
  if (error.isError()) {
    return error;
  }

  doc.remove(core_domain_key_);
  doc.remove(ws_url_path_key_);
  doc.remove(secure_url_key_);

  error = storeSecrets(doc);
  return error;
}

ErrorResult Storage::saveAuthToken(const char* token) {
  JsonDocument doc;
  ErrorResult error = loadSecrets(doc);
  if (error.isError()) {
    return error;
  }

  doc[ws_token_key_] = token;

  error = storeSecrets(doc);
  return error;
}

ErrorResult Storage::savePeripheral(const JsonObjectConst& peripheral) {
  JsonDocument peripherals_doc;
  ErrorResult error = loadPeripherals(peripherals_doc);
  if (error.isError()) {
    return error;
  }

  // Check if the peripheral already exists
  uint8_t i = 0;
  bool found = false;
  const char* peripheral_id =
      peripheral[peripheral::Peripheral::uuid_key_].as<const char*>();
  JsonArray peripherals = peripherals_doc.as<JsonArray>();
  for (JsonObject local_peri : peripherals) {
    if (strcmp(local_peri[peripheral::Peripheral::uuid_key_].as<const char*>(),
               peripheral_id) == 0) {
      found = true;
      break;
    }
    i++;
  }

  if (found) {
    // Yes: Replace item in array
    peripherals[i] = peripheral;
  } else {
    // No: Append to array
    peripherals.add(peripheral);
  }

  return storePeripherals(peripherals);
}

ErrorResult Storage::loadPeripherals(JsonDocument& peripherals_doc) {
  fs::File file = LittleFS.open(peripherals_path_, "r+", true);
  if (file.size() == 0) {
    return ErrorResult();
  }
  if (file) {
    DeserializationError error = deserializeJson(peripherals_doc, file);
    TRACEKJSON("Peris", peripherals_doc);
    file.close();
    if (error) {
      return ErrorResult(type_, F("Failed loading peripheral doc"));
    }
  }
  return ErrorResult();
}

ErrorResult Storage::storePeripherals(JsonArrayConst peripherals) {
  fs::File file = LittleFS.open(peripherals_path_, "w+", true);
  if (!file) {
    return ErrorResult(type_, F("Failed opening file"));
  }

  TRACELN(F("Saving param peris"));
  size_t bytes_written = serializeJson(peripherals, file);
  TRACEJSON(peripherals);

  file.close();
  if (bytes_written == 0 && peripherals.size()) {
    return ErrorResult(type_, F("Failed to write"));
  }
  return ErrorResult();
}

void Storage::deletePeripherals() { LittleFS.remove(peripherals_path_); }

void Storage::deletePeripheral(const char* peripheral_id) {
  JsonDocument peripherals_doc;
  ErrorResult error = loadPeripherals(peripherals_doc);
  if (error.isError()) {
    return;
  }
  JsonArray peripherals = peripherals_doc.as<JsonArray>();
  for (JsonObject i : peripherals) {
    if (strcmp(i[peripheral::Peripheral::uuid_key_].as<const char*>(),
               peripheral_id) == 0) {
      peripherals.remove(i);
    }
  }

  storePeripherals(peripherals_doc.as<JsonArrayConst>());
}

ErrorResult Storage::loadBehavior(JsonDocument& behavior_doc) {
  fs::File file = LittleFS.open(behavior_path_, "r+");
  if (file) {
    DeserializationError error = deserializeJson(behavior_doc, file);
    TRACEKJSON("Behav: ", behavior_doc);
    file.close();
    if (error) {
      return ErrorResult(type_, F("Failed loading behavior doc"));
    }
  }
  return ErrorResult();
}

ErrorResult Storage::storeBehavior(const JsonObjectConst& behavior) {
  fs::File file = LittleFS.open(behavior_path_, "w+");
  if (!file) {
    return ErrorResult(type_, F("Failed opening file"));
  }
  size_t bytes_written = serializeJson(behavior, file);
  TRACEKJSON("Behav: ", behavior);
  file.close();
  if (bytes_written == 0 && !behavior.isNull()) {
    return ErrorResult(type_, F("Failed to write"));
  }
  return ErrorResult();
}

void Storage::deleteBehavior() { LittleFS.remove(behavior_path_); }

ErrorResult Storage::loadCustomConfig(JsonDocument& config_doc) {
  fs::File file = LittleFS.open(custom_config_path_, "r+");
  if (file) {
    DeserializationError error = deserializeJson(config_doc, file);
    TRACEKJSON("Custom config: ", config_doc);
    file.close();
    if (error) {
      return ErrorResult(type_, F("Failed loading custom config doc"));
    }
  }
  return ErrorResult();
}

ErrorResult Storage::storeCustomConfig(const JsonObjectConst& config) {
  fs::File file = LittleFS.open(custom_config_path_, "w+");
  if (!file) {
    return ErrorResult(type_, F("Failed opening file"));
  }
  size_t bytes_written = serializeJson(config, file);
  TRACEKJSON("Custom config: ", config);
  file.close();
  if (bytes_written == 0 && !config.isNull()) {
    return ErrorResult(type_, F("Failed to write"));
  }
  return ErrorResult();
}

void Storage::deleteCustomConfig() { LittleFS.remove(custom_config_path_); }

const char* Storage::arduino_board_ = ARDUINO_BOARD;
const char* Storage::device_type_name_ = DEVICE_TYPE_NAME;
const char* Storage::device_type_id_ = DEVICE_TYPE_ID;

const char* Storage::core_domain_key_ = "core_domain";
const char* Storage::ws_url_path_key_ = "ws_url_path";
const char* Storage::secure_url_key_ = "secure_url";
const char* Storage::ws_token_key_ = "ws_token";

const char* Storage::wifi_aps_key_ = "wifi_aps";
const char* Storage::wifi_ap_ssid_key_ = "ssid";
const char* Storage::wifi_ap_password_key_ = "password";

const char* Storage::secrets_path_ = "/secrets.json";
const char* Storage::peripherals_path_ = "/peripherals.json";
const char* Storage::behavior_path_ = "/behavior.json";
const char* Storage::custom_config_path_ = "/custom_config.json";
const char* Storage::type_ = "storage";

}  // namespace inamata