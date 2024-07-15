#include "storage.h"

#include "peripheral/peripheral.h"

namespace inamata {

#if defined(ENABLE_TRACE)
void listDir(fs::FS& fs, const char* dirname, uint8_t levels) {
  Serial.printf("Listing directory: %s\r\n", dirname);

  File root = fs.open(dirname, "r+");
  if (!root) {
    Serial.println("- failed to open directory");
    return;
  }
  if (!root.isDirectory()) {
    Serial.println(" - not a directory");
    return;
  }

  File file = root.openNextFile();
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
        listDir(fs, file.name(), levels - 1);
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
#ifdef ESP32
  if (!LittleFS.begin()) {
    if (!LittleFS.begin(true)) {
      TRACELN(F("Failed mounting LittleFS"));
    } else {
      TRACELN(F("Formatted LittleFS on fail"));
    }
  }
#else
  if (!LittleFS.begin()) {
    TRACELN(F("Failed mounting LittleFS"));
  }
#endif
#ifdef ENABLE_TRACE
  listDir(LittleFS, "/", 1);
#endif
}

void Storage::closeFS() { LittleFS.end(); }

ErrorResult Storage::loadSecrets(JsonDocument& secrets_doc) {
  // Load a common config file for the subsystems
  fs::File secrets_file = LittleFS.open(secrets_path_, "r+");
  if (secrets_file) {
    DeserializationError error = deserializeJson(secrets_doc, secrets_file);
    secrets_file.close();
    if (error) {
      TRACEF("Failed parsing secrets.json: %s\n", error.c_str());
      return ErrorResult(type_, error.c_str());
    }
  } else {
    TRACELN(F("Failed opening (r+) secrets.json"));
  }
  return ErrorResult();
}

ErrorResult Storage::storeSecrets(JsonVariantConst secrets) {
  fs::File secrets_file = LittleFS.open(secrets_path_, "w+");
  if (!secrets_file) {
    return ErrorResult(type_, F("Failed opening (w+) secrets.json"));
  }

  size_t bytes_written = serializeJson(secrets, secrets_file);
  if (bytes_written == 0 && secrets.size()) {
    return ErrorResult(type_, F("Failed to write secrets"));
  }
  secrets_file.close();
#ifdef ENABLE_TRACE
  serializeJson(secrets, Serial);
#endif
  return ErrorResult();
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
  fs::File file = LittleFS.open(peripherals_path_, "r+");
  if (file) {
    DeserializationError error = deserializeJson(peripherals_doc, file);
    Serial.print("Loaded peris: ");
    serializeJson(peripherals_doc, Serial);
    Serial.println();
    file.close();
    if (error) {
      return ErrorResult(type_, F("Failed loading peripheral doc"));
    }
  }
  return ErrorResult();
}

ErrorResult Storage::storePeripherals(JsonArrayConst peripherals) {
  fs::File file = LittleFS.open(peripherals_path_, "w+");
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

const __FlashStringHelper* Storage::secrets_path_ = FPSTR("/secrets.json");
const __FlashStringHelper* Storage::peripherals_path_ =
    FPSTR("/peripherals.json");
const __FlashStringHelper* Storage::type_ = FPSTR("storage");

}  // namespace inamata