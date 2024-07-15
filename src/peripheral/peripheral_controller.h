#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>

#include <map>
#include <memory>

#include "managers/io_types.h"
#include "managers/service_getters.h"
#include "peripheral/peripheral.h"
#include "peripheral/peripheral_factory.h"
#include "utils/uuid.h"

namespace inamata {
namespace peripheral {

class PeripheralController {
 public:
  PeripheralController(PeripheralFactory& peripheral_factory);

  static const String& type();

  void setServices(ServiceGetters services);

  void handleCallback(const JsonObjectConst& message);

  /**
   * Returns a list of all peripherals' IDs and save version
   *
   * \return A list of all peripherals' IDs and versions
   */
  std::vector<utils::VersionedID> getPeripheralIDs();

  /**
   * Returns a shared pointer to the object or a nullptr if not found
   *
   * @param periphera_id ID of the peripheral to be found
   * @return A shared pointer of the object, or a nullptr if it does not exist
   */
  std::shared_ptr<Peripheral> getPeripheral(const utils::UUID& periphera_id);

  /**
   * Create a new peripheral according to the JSON doc
   *
   * \param doc The JSON doc with the parameters to create a peripheral
   * \return Contains the source and cause of the error, if it failed
   */
  ErrorResult add(const JsonObjectConst& doc);

  void sendBootErrors();

  std::vector<std::shared_ptr<Peripheral>> peripherals_;

  /// If an error occured during peripheral add on boot
  std::unique_ptr<ErrorResult> boot_add_error_;
  std::unique_ptr<utils::UUID> boot_add_error_peripheral_id_;

 private:
  /**
   *
   */
  void replacePeripherals(const JsonArrayConst& doc);

  /**
   * Remove a peripheral by its UUID
   *
   * \param doc The JSON doc containing the UUID of the peripheral to remove
   * \return Contains the source and cause of the error, if one occured
   */
  ErrorResult remove(const JsonObjectConst& doc);

  /// The server to which to reply to
  ServiceGetters services_;
  /// Map of UUIDs to their respective peripherals
  /// Factory to construct peripherals according to the JSON parameters
  PeripheralFactory& peripheral_factory_;

  static const __FlashStringHelper* peripheral_command_key_;
  static const __FlashStringHelper* sync_command_key_;
  static const __FlashStringHelper* add_command_key_;
  static const __FlashStringHelper* update_command_key_;
  static const __FlashStringHelper* remove_command_key_;
};

}  // namespace peripheral
}  // namespace inamata