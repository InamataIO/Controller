#include "peripheral_controller.h"

namespace inamata {
namespace peripheral {

PeripheralController::PeripheralController(
    peripheral::PeripheralFactory& peripheral_factory)
    : peripheral_factory_(peripheral_factory) {}

const String& PeripheralController::type() {
  static const String name{"PeripheralController"};
  return name;
}

void PeripheralController::setServices(ServiceGetters services) {
  services_ = services;
}

void PeripheralController::handleCallback(const JsonObjectConst& message) {
  if (message.isNull()) {
    sendBootErrors();
    return;
  }

  // Check if any peripheral commands have to be processed
  JsonVariantConst peripheral_commands = message[peripheral_command_key_];
  if (!peripheral_commands) {
    return;
  }
  TRACELN(F("Handling peripheral cmd"));

  // Replace stored peripherals with sync config
  JsonArrayConst sync_commands =
      peripheral_commands[sync_command_key_].as<JsonArrayConst>();
  if (sync_commands) {
    replacePeripherals(sync_commands);
    ESP.restart();
  }

  // Init the result doc with type and the request ID
  JsonDocument doc_out;
  doc_out[WebSocket::type_key_] = WebSocket::result_type_;
  JsonVariantConst request_id = message[WebSocket::request_id_key_];
  if (request_id) {
    doc_out[WebSocket::request_id_key_] = request_id;
  }
  JsonObject peripheral_results =
      doc_out[peripheral_command_key_].to<JsonObject>();

  // Add a peripheral for each command and store the result
  JsonArrayConst add_commands =
      peripheral_commands[add_command_key_].as<JsonArrayConst>();
  if (add_commands) {
    JsonArray add_results =
        peripheral_results[add_command_key_].to<JsonArray>();
    for (JsonVariantConst add_command : add_commands) {
      ErrorResult error = add(add_command);
      if (!error.isError()) {
        ErrorResult error = services_.getStorage()->savePeripheral(add_command);
        if (error.isError()) {
          Serial.printf("Saving err: %s\n", error.toString().c_str());
        }
      }
      WebSocket::addResultEntry(add_command[Peripheral::uuid_key_], error,
                                add_results);
    }
  }

  // Remove a peripheral for each command and store the result
  JsonArrayConst remove_commands =
      peripheral_commands[remove_command_key_].as<JsonArrayConst>();
  if (remove_commands) {
    JsonArray remove_results =
        peripheral_results[remove_command_key_].to<JsonArray>();
    for (JsonVariantConst remove_command : remove_commands) {
      ErrorResult error = remove(remove_command);
      if (!error.isError()) {
        const char* peripheral_id =
            remove_command[Peripheral::uuid_key_].as<const char*>();
        services_.getStorage()->deletePeripheral(peripheral_id);
      }
      WebSocket::addResultEntry(remove_command[Peripheral::uuid_key_], error,
                                remove_results);
    }
  }

  // Send the command results
  std::shared_ptr<WebSocket> web_socket = services_.getWebSocket();
  if (web_socket) {
    web_socket->sendResults(doc_out.as<JsonObject>());
  } else {
    TRACELN(ErrorResult(type(), ServiceGetters::web_socket_nullptr_error_)
                .toString());
  }
}

std::vector<utils::VersionedID> PeripheralController::getPeripheralIDs() {
  std::vector<utils::VersionedID> uuids;
  uuids.reserve(peripherals_.size());
  for (const auto& peripheral : peripherals_) {
    uuids.push_back({.id = peripheral->id, .version = peripheral->version});
  }
  return uuids;
}

ErrorResult PeripheralController::add(const JsonObjectConst& config) {
  utils::UUID peripheral_id(config[Peripheral::uuid_key_]);
  if (!peripheral_id.isValid()) {
    return ErrorResult(type(), Peripheral::uuid_key_error_);
  }

  // If the peripheral is present, try to replace it
  auto iterator =
      std::find_if(peripherals_.begin(), peripherals_.end(),
                   [&peripheral_id](const std::shared_ptr<Peripheral>& p) {
                     return p->id == peripheral_id;
                   });
  if (iterator != peripherals_.end()) {
    if (iterator->use_count() > 1) {
      return ErrorResult(type(), String(F("Peripheral still in use")));
    }
    peripherals_.erase(iterator);
  }

  // Try to create a new instance
  std::shared_ptr<peripheral::Peripheral> peripheral(
      peripheral_factory_.createPeripheral(services_, config));
  if (peripheral) {
    if (!peripheral->isValid()) {
      return peripheral->getError();
    }
  } else {
    return ErrorResult(type(), F("Error calling peripheral factory"));
  }

  peripherals_.emplace_back(peripheral);

  return ErrorResult();
}

void PeripheralController::sendBootErrors() {
  if (!boot_add_error_ || !boot_add_error_peripheral_id_) {
    return;
  }

  // Send peripheral boot error as result message
  JsonDocument doc_out;
  doc_out[WebSocket::type_key_] = WebSocket::result_type_;
  JsonObject peripheral_results =
      doc_out[peripheral_command_key_].to<JsonObject>();
  JsonArray add_results = peripheral_results[add_command_key_].to<JsonArray>();
  String peripheral_id = boot_add_error_peripheral_id_.get()->toString();
  WebSocket::addResultEntry(peripheral_id, *boot_add_error_.get(), add_results);
  services_.getWebSocket()->sendResults(doc_out.as<JsonObject>());

  // Clear boot errors
  boot_add_error_.reset();
  boot_add_error_peripheral_id_.reset();
}

void PeripheralController::replacePeripherals(const JsonArrayConst& configs) {
  // Overwrite saved peripherals with the sync data
  services_.getStorage()->storePeripherals(configs);
}

ErrorResult PeripheralController::remove(const JsonObjectConst& doc) {
  utils::UUID peripheral_id(doc[Peripheral::uuid_key_]);
  if (!peripheral_id.isValid()) {
    return ErrorResult(type(), Peripheral::uuid_key_error_);
  }

  auto iterator =
      std::find_if(peripherals_.begin(), peripherals_.end(),
                   [&peripheral_id](const std::shared_ptr<Peripheral>& p) {
                     return p->id == peripheral_id;
                   });
  if (iterator != peripherals_.end()) {
    if (iterator->use_count() > 1) {
      return ErrorResult(type(), String(F("Peripheral still in use")));
    }
    peripherals_.erase(iterator);
  }

  return ErrorResult();
}

std::shared_ptr<peripheral::Peripheral> PeripheralController::getPeripheral(
    const utils::UUID& peripheral_id) {
  auto iterator =
      std::find_if(peripherals_.begin(), peripherals_.end(),
                   [&peripheral_id](const std::shared_ptr<Peripheral>& p) {
                     return p->id == peripheral_id;
                   });
  if (iterator != peripherals_.end()) {
    return *iterator;
  } else {
    return std::shared_ptr<peripheral::Peripheral>();
  }
}

const __FlashStringHelper* PeripheralController::peripheral_command_key_ =
    FPSTR("peripheral");
const __FlashStringHelper* PeripheralController::sync_command_key_ =
    FPSTR("sync");
const __FlashStringHelper* PeripheralController::add_command_key_ =
    FPSTR("add");
const __FlashStringHelper* PeripheralController::update_command_key_ =
    FPSTR("update");
const __FlashStringHelper* PeripheralController::remove_command_key_ =
    FPSTR("remove");

}  // namespace peripheral
}  // namespace inamata
