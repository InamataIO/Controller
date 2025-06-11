#pragma once

#include <Arduino.h>

#include <memory>

#include "managers/storage.h"
#include "utils/person.h"

namespace inamata {

class ConfigManager {
 public:
  String init(std::shared_ptr<Storage> storage);
  void loop();

  const std::vector<Person> &getAllContacts() const;
  const String &getLocation() const;

  /**
   * Handle user data sent during Improv provisioning
   *
   * \return True if handled the data without error
   */
  bool handleImprovUserData(const JsonObjectConst &data);

  // Static function to display contact details.
  static void printPerson(const Person &info);

 private:
  enum class InputType {
    kAlphaNumericInput,  // Allows alphanumeric characters.
    kPhoneNumInput,      // Allows only characters which uses in phone numbers.
    kAnyInput,           // Allows any printable character.
    kNumberInput         // Allows only numbers.
  };

  enum class MenuState {
    kIdle,              // In idle mode (Main state machine is not yet started).
    kMainMenu,          // Main selection state.
    kAddContact,        // Add contact state.
    kEditContact,       // Edit contact state.
    kDeleteContact,     // Delete contact state.
    kDateTimeSet,       // Set date and time state.
    kEditLocationName,  // Edit location name state.
    kFactoryReset,      // Clear all data and factory reset device
    kExit               // Exit from configuration mode.
  };

  enum class EditState {
    kEditInit,  // Start editing.
    kEditing    // In the middle of editing state.
  };

  enum class SelectionType {
    kSelecting,   // User is still selecting the option from list.
    kSelected,    // User selected an option from the list.
    kNotSelected  // Cancel selection process.
  };

  enum class ContactState {
    kInit,              // Initializing state.
    kNameSelect,        // Name selection state (in edit or delete flows).
    kNameInput,         // Name input state.
    kPhoneNumberInput,  // Input phone number state.
    kGroupDataMaintenanceInput,  // Setting up maintenance flag.
    kGroupDataManagementInput,   // Setting up management flag.
    kGroupDataStatisticsInput    // Setting up statistics flag.
  };

  enum class LocationEditState {
    kLocationInit,  // Initial state.
    kLocationInput  // Input location name state.
  };

  enum class FactoryResetState { kConfirmInit, kConfirmInput };

  enum class SetDateTimeStage {
    SDT_INIT,    // Initial state.
    SDT_YEAR,    // Year input state.
    SDT_MONTH,   // Month input state.
    SDT_DAY,     // Day input state.
    SDT_HOUR,    // Hour input state.
    SDT_MINUTE,  // Minute input state.
    SDT_SECOND   // Second input state.
  };

  void printMenu();
  void resetSubState();
  void handleInput(char input);

  void showAllContacts();
  bool addContact(char key);
  void clearContactNameInEditMode(String &name);
  SelectionType nameSelection(char key, String &last_name);
  bool editContact(char key);

  bool editContactField(String &receipent_data, InputType type, char in_key);

  bool processInputBuffer(InputType type, char inKey, String &output);
  bool getGroupDataInput(char in_key, uint8_t group);
  bool isValidChar(InputType type, char c);

  bool deleteContact(char key);

  bool setSystemDateTime(char key);

  bool editLocationName(char key);

  bool factoryReset(char key);

  void parseConfig(JsonObjectConst config);
  void saveConfig();

  static bool isValidLocation(const String &location);

  // configuration manager's serial operations.
  String input_buffer_;         // Buffer to store raw user input.
  int selected_buffer_;         // Buffer to store selected contact index (in
                                // edit and delete modes).
  String selected_name_buffer;  // Buffer to store selected contact name.
  Person selected_contact_;
  String selection_to_edit_;  // Buffer to store the selected contact to edit.
  EditState edit_mode_;
  PersonManager person_manager_;
  LocationEditState location_edit_state_;  // State for editing the location.
  FactoryResetState factory_reset_state_;  // State of reseting device

  String temp_name_;      // Name of the contact.
  String temp_contact_;   // Phone number of the contact.
  String temp_number_;    // Temporary buffer to store the numbers input by the
                          // user (mainly used in date/time input routines).
  String temp_location_;  // Buffer to store user input location name input.
  String temp_factory_reset_;  // Buffer to store confirm factory reset
  Person::GroupData temp_group_data_;

  MenuState menu_state_;             // Current state of the menu.
  ContactState contact_menu_state_;  // Current state of the add contact menu.

  SetDateTimeStage
      date_input_stage_;  // Current stage of the date/time input process.
  int input_year_, input_month_, input_day_, input_hour_, input_minute_,
      input_second_;  // Components of date/time input flow.

  String location_;

  std::shared_ptr<Storage> storage_;

  static const uint8_t kMaxLocationLength = 30;
};

}  // namespace inamata
