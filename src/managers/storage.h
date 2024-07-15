#pragma once

#include <ArduinoJson.h>
#include <LittleFS.h>

#include "managers/logging.h"
#include "managers/types.h"

namespace inamata {

class Storage {
 public:
  void openFS();
  void closeFS();

  /**
   * Load stored secrets into passed JSON doc
   *
   * \param secrets_doc The JSON doc to load secrets into
   * \return If an error occured during deserialization. No error if no file
   */
  ErrorResult loadSecrets(JsonDocument& secrets_doc);

  /**
   * Store secrets JSON doc on the file system
   *
   * \param secrets_doc JSON doc with the secrets to be stored
   */
  ErrorResult storeSecrets(JsonVariantConst secrets);

  /**
   * Load saved peripherals to JSON doc
   *
   * \return Error if one occured
   */
  ErrorResult loadPeripherals(JsonDocument& peripheral_doc);

  /**
   * Store passed peripherals or private JSON doc on flash storage
   *
   * \return Error if one occured
   */
  ErrorResult storePeripherals(JsonArrayConst peripherals);

  /**
   * Saves a single peripheral to flash storage
   *
   * \param doc The peripheral to be saved. Overwrites old version if it exists
   * \return If an error occured
   */
  ErrorResult savePeripheral(const JsonObjectConst& peripheral);

  /**
   * Deletes stored peripherals
   */
  void deletePeripherals();

  /**
   * Removes a single peripheral from flash storage
   *
   * \param peripheral_id The ID of the peripheral
   */
  void deletePeripheral(const char* peripheral_id);

 private:
  static const __FlashStringHelper* secrets_path_;
  static const __FlashStringHelper* peripherals_path_;
  static const __FlashStringHelper* type_;
};

}  // namespace inamata
