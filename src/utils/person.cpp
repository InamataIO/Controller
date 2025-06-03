#include "person.h"

#include <Arduino.h>

#include <cstring>
#include <vector>

namespace inamata {

/**
 * \brief Initializes the people list by clearing any existing data.
 *
 * This function clears the `people_`, effectively resetting the list of people.
 * It is typically called when the list needs to be reset before adding new data
 * or starting a fresh operation.
 *
 * \note This function does not allocate any new memory or modify the structure
 * of the list; it only removes all existing entries.
 */
void PersonManager::initializeList() { people_.clear(); }

/**
 * \brief Returns the current count of person entries in the list.
 *
 * This function returns the number of entries currently stored in the
 * `people_`. It provides a way to check how many personal person records are
 * available.
 *
 * \return The size of the `people_`, representing the total number of
 * personal information entries.
 */
int PersonManager::getCount() const { return people_.size(); }

/**
 * \brief Returns the list of people
 *
 * \return A const reference to the list of people
 */
const std::vector<Person> &PersonManager::getAllPeople() const {
  return people_;
}

/**
 * \brief Adds a new phone_number to the list.
 *
 * This function adds a new entry to the `people_` containing the phone_number's
 * name, phone number, site name, and associated group data. If the list has
 * reached its maximum allowed size (`Person::kMaxEntries`), the addition
 * is rejected.
 *
 * \param name The full name of the phone_number.
 * \param phone_number The phone number of the contact.
 * \param group_data A character representing the group data flags (e.g.,
 * maintenance, management, statistics).
 *
 * \return The new size of the `people_` if the contact is successfully
 * added, or -1 if the list is already at the maximum capacity.
 */
int PersonManager::add(const String &name, const String &phone_number,
                       Person::GroupData group_data) {
  if (people_.size() >= Person::kMaxEntries) {
    return -1;
  }

  people_.push_back({name, phone_number, group_data});
  return people_.size();
}

/**
 * \brief Edits the information of an existing contact in the list.
 *
 * This function searches for a contact by their original name and updates
 * their personal information (name, phone_number, site, and group data) with
 * the provided new values. If the contact with the specified name is found,
 * the information is updated. If the contact is not found, no changes are
 * made.
 *
 * \param name The current name of the contact whose information is to be
 * edited.
 * \param new_name The new name for the contact.
 * \param new_contact The new phone number for the contact.
 * \param new_group_data The updated group data flags (e.g., maintenance, ...).
 */
void PersonManager::edit(const String &name, const String &new_name,
                         const String &new_contact,
                         Person::GroupData new_group_data) {
  for (auto &person : people_) {
    if (person.name.equalsIgnoreCase(name)) {
      person.name = new_name;
      person.phone_number = new_contact;
      person.group_data = new_group_data;
      return;
    }
  }
}

/**
 * \brief Deletes a contact from the list based on their name.
 *
 * This function searches for a contact with the specified name in the list
 * and removes the contact's entry if found. It uses the `std::remove_if`
 * algorithm to locate the contact and then erases the corresponding element
 * from the list. The function returns the updated size of the list after the
 * deletion process.
 *
 * \param name The name of the contact whose information is to be deleted.
 *
 * \return The updated size of the list after the contact has been removed.
 *         If no contact is found, the list size remains unchanged.
 */
int PersonManager::remove(const String &name) {
  people_.erase(std::remove_if(people_.begin(), people_.end(),
                               [&](Person &person) {
                                 return strcasecmp(person.name.c_str(),
                                                   name.c_str()) == 0;
                               }),
                people_.end());

  return people_.size();
}

/**
 * \brief Remove all people
 */
void PersonManager::clear() { people_.clear(); }

/**
 * \brief Searches for a contact by name in the list.
 *
 * This function performs a case-insensitive search for a contact's name in
 * the list. The provided name is converted to lowercase, and each contact's
 * name in the list is also converted to lowercase for comparison. If a match
 * is found, a pointer to the corresponding `Person` is returned. If no
 * match is found, NULL (`nullptr`) is returned.
 *
 * \param name The name of the contact to search for.
 *
 * \return A pointer to the `Person` if a matching name is found,
 *         otherwise `nullptr`.
 */
const Person *PersonManager::search(const String &name) const {
  for (auto &person : people_) {
    if (person.name.equalsIgnoreCase(name)) {
      return &person;
    }
  }

  return nullptr;
}

/**
 * \brief Parses a JSON object people.
 *
 * Expects a JSON array with objects in the format:
 *   [{"name": "...", "phone": "...", "groups": 0-7}]
 * Will replace duplicate names with the last one in the list. The groups are
 * parsed by bit order, maintenance, management and statistics group.
 */
void PersonManager::parseJson(JsonArrayConst people_json) {
  for (JsonObjectConst person : people_json) {
    remove(person["name"]);
    add(person["name"], person["phone"], person["groups"].as<u_long>());
  }
}

/**
 * \brief Serializes saved people to a JSON array.
 *
 * Will produce a JSON array with objects in the format:
 *   [{"name": "...", "phone": "...", "groups": 0-7}]
 * The groups are bit ordered as, maintenance, management and statistics.
 */
void PersonManager::serializeJson(JsonArray people_json) const {
  for (const Person &person : people_) {
    JsonObject person_json = people_json.add<JsonObject>();
    person_json["name"] = person.name;
    person_json["phone"] = person.phone_number;
    person_json["groups"] = person.group_data.to_ulong();
  }
}

/**
 * \brief Checks if a duplicate entry exists in the list.
 *
 * This function checks if a contact with the given new name already exists
 * in the list, excluding the contact with the specified old name. It is
 * useful for ensuring that contact names are unique when editing an existing
 * entry.
 *
 * \param old_name The current name of the contact (to be excluded from the
 *                comparison).
 * \param new_name The new name to check for duplicates.
 *
 * \return `true` if a contact with the new name exists and is not the same
 *         as the old name, otherwise `false`.
 */
bool PersonManager::isDuplicateEntryExists(const String &old_name,
                                           const String &new_name) const {
  for (const auto &person : people_) {
    if (person.name == new_name && person.name != old_name) {
      return true;
    }
  }

  return false;
}

/**
 * \brief Checks if a phone_number number already exists in the list.
 *
 * This function checks if a contact with the given new phone_number number
 * already exists in the list, excluding the contact with the specified old
 * phone_number number. It is useful for ensuring that phone_number numbers are
 * unique when editing an existing contact.
 *
 * \param old_number The current phone_number number of the contact (to be
 * excluded from the comparison).
 * \param new_number The new phone_number number to check for duplicates.
 *
 * \return `true` if a contact with the new phone_number number exists and is
 * not the same as the old phone_number number, otherwise `false`.
 */
bool PersonManager::isNumberExists(const String &old_number,
                                   const String &new_number) const {
  for (const auto &person : people_) {
    if (person.phone_number == new_number &&
        person.phone_number != old_number) {
      return true;
    }
  }

  return false;
}

/**
 * \brief Prints all contact information from the list.
 *
 * This function iterates over the list of contacts and displays each
 * contact's information using the provided callback function. The callback
 * allows for customizable display formatting.
 *
 * \param print_person_cb The callback used to print a person's details
 */
void PersonManager::printAll(printPerson print_person_cb) const {
  for (const auto &person : people_) {
    print_person_cb(person);
  }
}

/**
 * \brief Retrieves the name of a contact at the specified position in the
 * list.
 *
 * This function checks if the given position is valid within the list. If the
 * position is valid, it assigns the contact's name to the provided reference
 * parameter and returns true. If the position is invalid, it returns false.
 *
 * \param position The index of the contact whose name is to be retrieved.
 * \param name A reference to a `String` where the contact's name will be
 * assigned if the position is valid.
 *
 * \return A `bool` indicating whether the name was successfully retrieved.
 * Returns true if the position is valid and the name is assigned, or false if
 * the position is out of bounds.
 */
bool PersonManager::getNameAtPosition(size_t position, String &name) {
  if (position < people_.size()) {
    name = people_[position].name;
    return true;
  }
  return false;
}

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
bool PersonManager::isValidName(const String &name) {
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
 * Valid:
 *   012345678
 *   +4912391239
 *   0(12)9870992
 *   0(12 )-9870992
 *   0(1) )2)9870992
 *
 * Invalid:
 *   +(4567890987)
 *   ( 98798798729
 *   -81981239812
 *    81981239812
 *   )81981239812
 *
 * \param number The phone number string to be validated.
 * \return `true` if the phone number contains only valid characters and has
 * between 7 and 15 digits, `false` otherwise.
 */
bool PersonManager::isValidPhoneNumber(const String &number) {
  int digit_count = 0;
  for (size_t i = 0; i < number.length(); i++) {
    char c = number[i];
    if (isDigit(c)) {
      digit_count++;
    } else if (i == 0) {
      // Valid special first characters
      if (!(c == '+' || c == '(')) {
        return false;
      }
    } else {
      // Valid special characters after first character and atleast one digit
      if (!(c == '(' || c == '-' || c == ' ' || c == ')') || !digit_count) {
        return false;
      }
    }
  }
  return (digit_count >= 7 && digit_count <= 15);
}

}  // namespace inamata