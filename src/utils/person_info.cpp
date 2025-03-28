#include "person_info.h"

#include <Arduino.h>

#include <cstring>
#include <vector>

namespace inamata {

/**
 * \brief Initializes the personal info list by clearing any existing data.
 *
 * This function clears the `infoList`, effectively resetting the list of
 * personal information. It is typically called when the list needs to be reset
 * before adding new data or starting a fresh operation.
 *
 * \note This function does not allocate any new memory or modify the structure
 * of the list; it only removes all existing entries.
 */
void PersonalInfoManager::initializeList() { infoList.clear(); }

/**
 * \brief Returns the current count of personal information entries in the list.
 *
 * This function returns the number of entries currently stored in the
 * `infoList`. It provides a way to check how many personal info records are
 * available.
 *
 * \return The size of the `infoList`, representing the total number of personal
 *         information entries.
 */
int PersonalInfoManager::getPersonalInfoCount() { return infoList.size(); }

/**
 * \brief Adds a new recipient's personal information to the list.
 *
 * This function adds a new entry to the `infoList` containing the recipient's
 * name, contact number, site name, and associated group data. If the list has
 * reached its maximum allowed size (`MAX_PERSONAL_INFO_COUNT`), the addition is
 * rejected.
 *
 * \param name The full name of the recipient.
 * \param contact The contact number of the recipient.
 * \param site The site name associated with the recipient.
 * \param groupData A character representing the group data flags (e.g.,
 * maintenance, management, statistics).
 *
 * \return The new size of the `infoList` if the recipient is successfully
 * added, or -1 if the list is already at the maximum capacity.
 */
int PersonalInfoManager::addInfo(const String &name, const String &contact,
                                 const String &site, char groupData) {
  if (infoList.size() >= MAX_PERSONAL_INFO_COUNT) {
    return -1;
  }

  infoList.push_back({name, contact, site, groupData});
  return infoList.size();
}

/**
 * \brief Edits the information of an existing recipient in the list.
 *
 * This function searches for a recipient by their original name and updates
 * their personal information (name, contact, site, and group data) with the
 * provided new values. If the recipient with the specified name is found, the
 * information is updated. If the recipient is not found, no changes are made.
 *
 * \param name The current name of the recipient whose information is to be
 * edited.
 * \param newName The new name for the recipient.
 * \param newContact The new contact number for the recipient.
 * \param newSite The new site name associated with the recipient.
 * \param newGroupData A character representing the updated group data flags
 * (e.g., maintenance, management, statistics).
 */
void PersonalInfoManager::editInfo(const String &name, const String &newName,
                                   const String &newContact,
                                   const String &newSite, char newGroupData) {
  for (auto &info : infoList) {
    if (strcasecmp(info.name.c_str(), name.c_str()) == 0) {
      info.name = newName;
      info.contactNumber = newContact;
      info.siteName = newSite;
      info.groupData = newGroupData;
      return;
    }
  }
}

/**
 * \brief Deletes a recipient's information from the list based on their name.
 *
 * This function searches for a recipient with the specified name in the list
 * and removes the recipient's entry if found. It uses the `std::remove_if`
 * algorithm to locate the recipient and then erases the corresponding element
 * from the list. The function returns the updated size of the list after the
 * deletion process.
 *
 * \param name The name of the recipient whose information is to be deleted.
 *
 * \return The updated size of the list after the recipient has been removed.
 *         If no recipient is found, the list size remains unchanged.
 */
int PersonalInfoManager::deleteInfo(const String &name) {
  infoList.erase(std::remove_if(infoList.begin(), infoList.end(),
                                [&](RecipientData &info) {
                                  return strcasecmp(info.name.c_str(),
                                                    name.c_str()) == 0;
                                }),
                 infoList.end());

  return infoList.size();
}

/**
 * \brief Searches for a recipient by name in the list.
 *
 * This function performs a case-insensitive search for a recipient's name in
 * the list. The provided name is converted to lowercase, and each recipient's
 * name in the list is also converted to lowercase for comparison. If a match
 * is found, a pointer to the corresponding `RecipientData` is returned. If no
 * match is found, NULL (`nullptr`) is returned.
 *
 * \param name The name of the recipient to search for.
 *
 * \return A pointer to the `RecipientData` if a matching name is found,
 *         otherwise `nullptr`.
 */
RecipientData *PersonalInfoManager::searchInfo(const String &name) {
  String searchName(name);

  for (auto &info : infoList) {
    String infoName(info.name);

    if (strcasecmp(infoName.c_str(), searchName.c_str()) == 0) {
      return &info;
    }
  }

  return nullptr;
}

/**
 * \brief Checks if a duplicate entry exists in the list.
 *
 * This function checks if a recipient with the given new name already exists
 * in the list, excluding the recipient with the specified old name. It is
 * useful for ensuring that recipient names are unique when editing an existing
 * entry.
 *
 * \param oldName The current name of the recipient (to be excluded from the
 *                comparison).
 * \param newName The new name to check for duplicates.
 *
 * \return `true` if a recipient with the new name exists and is not the same
 *         as the old name, otherwise `false`.
 */
bool PersonalInfoManager::isDuplicateEntryExists(const String &oldName,
                                                 const String &newName) {
  for (const auto &info : infoList) {
    if (strcmp(info.name.c_str(), newName.c_str()) == 0 &&
        strcmp(info.name.c_str(), oldName.c_str()) != 0) {
      return true;
    }
  }

  return false;
}

/**
 * \brief Checks if a contact number already exists in the list.
 *
 * This function checks if a recipient with the given new contact number
 * already exists in the list, excluding the recipient with the specified old
 * contact number. It is useful for ensuring that contact numbers are unique
 * when editing an existing recipient.
 *
 * \param oldContact The current contact number of the recipient (to be excluded
 *                   from the comparison).
 * \param newContact The new contact number to check for duplicates.
 *
 * \return `true` if a recipient with the new contact number exists and is not
 *         the same as the old contact number, otherwise `false`.
 */
bool PersonalInfoManager::isNumberExists(const String &oldContact,
                                         const String &newContact) {
  for (const auto &info : infoList) {
    if (strcmp(info.contactNumber.c_str(), newContact.c_str()) == 0 &&
        strcmp(info.contactNumber.c_str(), oldContact.c_str()) != 0) {
      return true;
    }
  }

  return false;
}

/**
 * \brief Prints all recipient information from the list.
 *
 * This function iterates over the list of recipients and displays each
 * recipient's information using the provided callback function. The callback
 * allows for customizable display formatting.
 *
 * \param serial The serial interface used to print recipient information.
 * \param infoDisplayCallback The callback function used to display individual
 * recipient information. It should accept a serial interface and a
 *                            `RecipientData` object as parameters.
 */
void PersonalInfoManager::printAll(PersonInfoDisplay infoDisplayCallback) {
  for (const auto &info : infoList) {
    infoDisplayCallback(info);
  }
}

/**
 * \brief Retrieves the name of a recipient at the specified position in the
 * list.
 *
 * This function checks if the given position is valid within the list. If the
 * position is valid, it assigns the recipient's name to the provided reference
 * parameter and returns true. If the position is invalid, it returns false.
 *
 * \param position The index of the recipient whose name is to be retrieved.
 * \param name A reference to a `String` where the recipient's name will be
 * assigned if the position is valid.
 *
 * \return A `bool` indicating whether the name was successfully retrieved.
 * Returns true if the position is valid and the name is assigned, or false if
 * the position is out of bounds.
 */
bool PersonalInfoManager::getNameAtPosition(size_t position, String &name) {
  if (position < infoList.size()) {
    name = infoList[position].name;
    return true;
  }
  return false;
}

}  // namespace inamata