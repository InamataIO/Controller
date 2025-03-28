#include "person_info_validator.h"

#include <Arduino.h>

namespace inamata {

/**
 * \brief Validates if a given name is valid based on specified criteria.
 *
 * This function checks if the provided name consists only of alphabetic
 * characters, spaces, hyphens, or apostrophes. Additionally, it ensures that
 * the name is not empty. The validation is designed to allow names with special
 * characters like hyphens and apostrophes (e.g., "O'Neil" or "Sunil-Perera"),
 * but restricts any other characters.
 *
 * \param name The name string to be validated.
 * \return `true` if the name is valid (contains only allowed characters and is
 * not empty), `false` otherwise.
 */
bool Validator::isValidName(const String &name) {
  for (size_t i = 0; i < name.length(); i++) {
    char c = name[i];
    if (!(isAlpha(c) || c == ' ' || c == '-' || c == '\'')) {
      return false;
    }
  }
  return !name.isEmpty();
}

/**
 * \brief Validates if a given phone number is valid based on specified
 * criteria.
 *
 * This function checks if the provided phone number contains only valid
 * characters: digits, plus signs, hyphens, spaces, and parentheses. The number
 * must also contain between 7 and 15 digits (inclusive), ensuring a reasonable
 * length for phone numbers. It supports ITU formats, allowing characters such
 * as `+`, `-`, `()`, and spaces to separate the digits.
 *
 * \param phone The phone number string to be validated.
 * \return `true` if the phone number contains only valid characters and has
 * between 7 and 15 digits, `false` otherwise.
 */
bool Validator::isValidPhoneNumber(const String &phone) {
  int digitCount = 0;
  for (size_t i = 0; i < phone.length(); i++) {
    char c = phone[i];
    if (isDigit(c)) {
      digitCount++;
    } else if (!(c == '+' || c == '-' || c == ' ' || c == '(' || c == ')')) {
      return false;
    }
  }
  return (digitCount >= 7 && digitCount <= 15);
}

/**
 * \brief Validates if a given place name is valid based on specified criteria.
 *
 * This function checks if the provided place name contains only valid
 * characters: alphabets, spaces, hyphens, and apostrophes. The name must also
 * not be empty. This function is useful for ensuring that place names conform
 * to typical formats (e.g., "New York", "St. John's").
 *
 * \param place The place name string to be validated.
 * \return `true` if the place name contains only valid characters and is not
 * empty, `false` otherwise.
 */
bool Validator::isValidPlaceName(const String &place) {
  for (size_t i = 0; i < place.length(); i++) {
    char c = place[i];
    if (!(isAlpha(c) || c == ' ' || c == '-' || c == '\'')) {
      return false;
    }
  }
  return !place.isEmpty();
}

}  // namespace inamata