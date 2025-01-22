#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>

#include <array>

#include "managers/types.h"
#include "utils/uuid.h"

namespace inamata {
namespace peripheral {

class TaskFactory;

class Peripheral {
 public:
  Peripheral() = default;
  virtual ~Peripheral() = default;

  /**
   * Gives the type of the peripheral
   *
   * Overwritten by each concrete peripheral instantiation and used by the
   * peripheral polymorphism system.
   *
   * \return The name of the type
   */
  virtual const String& getType() const = 0;

  /**
   * Checks if the peripheral is valid (often used after construction)
   *
   * \return True if it is valid
   */
  bool isValid() const;

  /**
   * Returns the error result which contains the cause of the errro
   *
   * \return The error result
   */
  ErrorResult getError() const;

  /**
   * Converts JsonVariant pin number to int
   *
   * \return Pin number on valid pin or -1 on error
   */
  static int toPin(JsonVariantConst pin);

  static String peripheralNotFoundError(const utils::UUID& uuid);

  /**
   * Generate "<UUID> not a valid <type>" error
   *
   * \return Error string with UUID and type
   */
  static String notAValidError(const utils::UUID& uuid, const String& type);

  /// ID of the peripheral
  utils::UUID id{nullptr};
  /// Version tracking config changes. Incremented on each save / change
  int version = -1;

  static const __FlashStringHelper* uuid_key_;
  static const __FlashStringHelper* uuid_key_error_;

 protected:
  /**
   * Mark the peripheral as invalid and should therefore not be used
   *
   * \see isValid()
   */
  void setInvalid();

  /**
   * Mark the peripheral as invalid and give the reason for the error
   *
   * The state can be checked with the isValid function and the reason can be
   * extracted with the getError() function.
   *
   * \see isValid() and getError()
   * \param error_message The reason for being marked invalid
   */
  void setInvalid(const String& error_message);

  /**
   * Returns either data point type key's value or an invalid UUID
   *
   * \param parameters The peripheral's parameters
   * \return The stored DPT UUID or an invalid UUID
   */
  utils::UUID getDataPointType(const JsonObjectConst& parameters);

  // Common parameter keys can be reused
  static const __FlashStringHelper* data_point_type_key_;
  static const __FlashStringHelper* data_point_type_short_key_;
  static const __FlashStringHelper* data_point_type_key_error_;
  static const __FlashStringHelper* temperature_data_point_type_key_;
  static const __FlashStringHelper* temperature_data_point_type_key_error_;
  static const __FlashStringHelper* pressure_data_point_type_key_;
  static const __FlashStringHelper* pressure_data_point_type_key_error_;
  static const __FlashStringHelper* humidity_data_point_type_key_;
  static const __FlashStringHelper* humidity_data_point_type_key_error_;

  static const __FlashStringHelper* pin_key_;
  static const __FlashStringHelper* pin_key_error_;
  static const __FlashStringHelper* active_low_key_;
  static const __FlashStringHelper* active_low_key_error_;
  static const __FlashStringHelper* temperature_c_key_;
  static const __FlashStringHelper* temperature_c_key_error_;
  static const __FlashStringHelper* humidity_rh_key_;
  static const __FlashStringHelper* humidity_rh_key_error_;

  static const __FlashStringHelper* variant_key_;
  static const __FlashStringHelper* variant_key_error_;

  static const __FlashStringHelper* rx_key_;
  static const __FlashStringHelper* tx_key_;
  static const __FlashStringHelper* dere_key_;
  static const __FlashStringHelper* baud_rate_key_;
  static const __FlashStringHelper* config_key_;
  static const __FlashStringHelper* config_error_;
  static const __FlashStringHelper* modbus_client_adapter_key_;

 private:
  /// If the peripheral was constructed correctly and is still functional
  bool valid_ = true;
  /// The error message if an error occured
  String error_message_;

  static const __FlashStringHelper* peripheral_not_found_error_;
};

}  // namespace peripheral
}  // namespace inamata
