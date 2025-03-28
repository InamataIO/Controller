#pragma once

#include <Arduino.h>

#include <vector>

#include "recipient_data_def.h"

namespace inamata {

class PersonalInfoManager {
 private:
  std::vector<RecipientData> infoList;  // List of all personal info entries.

 public:
  // Callback type for displaying personal info.
  using PersonInfoDisplay = void (*)(const RecipientData &info);

  void initializeList();
  int getPersonalInfoCount();

  int addInfo(const String &name, const String &contact, const String &site,
              char groupData);
  void editInfo(const String &name, const String &newName,
                const String &newContact, const String &newSite,
                char newGroupData);
  int deleteInfo(const String &name);

  RecipientData *searchInfo(const String &name);

  bool isDuplicateEntryExists(const String &oldName, const String &newName);
  bool isNumberExists(const String &oldContact, const String &newContact);

  void printAll(PersonInfoDisplay infoDisplayCallback);

  bool getNameAtPosition(size_t position, String &name);
};

}  // namespace inamata