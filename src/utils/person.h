#pragma once

#include <ArduinoJson.h>

#include <bitset>
#include <vector>

namespace inamata {

enum GroupDataOffset {
  kGroupDataMaintenanceBit,
  kGroupDataManagementBit,
  kGroupDataStatisticsBit,
  kGroupDataCount,
};

// Structure to hold contact information.
struct Person {
  using GroupData = std::bitset<kGroupDataCount>;

  /// @brief Removes non-digit (except +) chars from number
  /// @return Cleaned phone number for use with TinyGSM
  String cleanPhoneNumber() const {
    String cleaned;
    cleaned.reserve(phone_number.length());
    for (const char c : phone_number) {
      if (isDigit(c) || c == '+') {
        cleaned += c;
      }
    }
    return cleaned;
  }

  String name;
  String phone_number;

  // Group data as a bit field (maintenance, management, statistics)
  GroupData group_data;

  // Maximum number of person information entries.
  static const uint8_t kMaxEntries = 10;
};

class PersonManager {
 public:
  /// Callback type for displaying personal info.
  using printPerson = void (*)(const Person &info);

  void initializeList();
  int getCount() const;
  const std::vector<Person> &getAllPeople() const;

  int add(const String &name, const String &contact,
          Person::GroupData group_data);
  void edit(const String &name, const String &newName,
            const String &new_contact, Person::GroupData new_group_data);
  int remove(const String &name);
  void clear();

  const Person *search(const String &name) const;

  void parseJson(JsonArrayConst people);
  void serializeJson(JsonArray people) const;

  bool isDuplicateEntryExists(const String &old_name,
                              const String &new_name) const;
  bool isNumberExists(const String &old_contact,
                      const String &new_contact) const;

  void printAll(printPerson print_person_cb) const;

  bool getNameAtPosition(size_t position, String &name);

  static bool isValidName(const String &name);
  static bool isValidPhoneNumber(const String &phone);

 private:
  std::vector<Person> people_;
};

}  // namespace inamata