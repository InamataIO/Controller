#include "config_menu.h"

#include <Arduino.h>

#include "managers/log_manager.h"
#include "managers/services.h"
#include "managers/time_manager.h"
#include "utils/person.h"

namespace inamata {

String ConfigManager::init(std::shared_ptr<Storage> storage) {
  resetSubState();
  menu_state_ = MenuState::kIdle;

  if (!storage) {
    return ServiceGetters::storage_nullptr_error_;
  }
  storage_ = storage;
  JsonDocument config;
  ErrorResult error = storage_->loadCustomConfig(config);
  if (!error.isError() && !config.isNull()) {
    parseConfig(config.as<JsonObjectConst>());
  }

  Serial.println("Press [C] to enter configuration menu.");
  return "";
}

/**
 * \brief Handles the main configuration loop, processing user input via Serial.
 *
 * This function continuously checks for serial input and manages state
 * transitions within the configuration menu system. It processes user commands,
 * executes corresponding actions, and updates the menu state accordingly.
 *
 * Functionality:
 * - If the device is in IDLE state and receives 'C' or 'c', it enters config
 * mode.
 * - In MAIN_MENU state, it processes menu selections.
 * - In other states (ADD, EDIT, REMOVE contact), it handles the
 * respective actions.
 * - If a stateful operation completes, the menu is reprinted for user guidance.
 *
 * \note This function should be called repeatedly to ensure continuous
 * interaction.
 */
void ConfigManager::loop() {
  bool is_still_in_state = false;

  if (Serial.available()) {
    char key = Serial.read();

    if (menu_state_ == MenuState::kIdle) {
      if ((key == 'C') || (key == 'c')) {  // 'C' key enters config mode
        printMenu();
        return;
      }
    } else if (menu_state_ == MenuState::kMainMenu) {
      if (!(key == '\r' || key == '\n')) {
        Serial.print(key);
        Serial.println("\r\n");
        handleInput(key);
      }

      return;
    } else if (menu_state_ == MenuState::kAddContact) {
      is_still_in_state = addContact(key);
    } else if (menu_state_ == MenuState::kEditContact) {
      is_still_in_state = editContact(key);
    } else if (menu_state_ == MenuState::kDeleteContact) {
      is_still_in_state = deleteContact(key);
    }
#ifdef RTC_MANAGER
    else if (menu_state_ == MenuState::kDateTimeSet) {
      is_still_in_state = setSystemDateTime(key);
    }
#endif
    else if (menu_state_ == MenuState::kEditLocationName) {
      is_still_in_state = editLocationName(key);
    }
    // Show main menu if state machine is terminated or in idle mode.
    if (!is_still_in_state) {
      printMenu();
    }
  }
}

/**
 * \brief Validates if a given character is allowed based on the specified input
 * type.
 *
 * This function checks whether the provided character `c` is valid according to
 * the given `InputType`. It ensures user inputs meet the expected format.
 *
 * \param type The expected input type (ALPHA_NUMERIC_INPUT, NUMERIC_INPUT,
 * ANY_INPUT).
 * \param c The character to validate.
 * \return true if the character is valid for the specified input type, false
 * otherwise.
 *
 * Functionality:
 * - ALPHA_NUMERIC_INPUT: Allows letters, digits, and spaces.
 * - NUMERIC_INPUT: Allows only digits.
 * - ANY_INPUT: Allows any printable character.
 */
bool ConfigManager::isValidChar(InputType type, char c) {
  switch (type) {
    case InputType::kAlphaNumericInput:
      return isalnum(c) || c == ' ';
    case InputType::kPhoneNumInput:
      return isdigit(c) || c == '+' || c == ' ' || c == '(' || c == ')';
    case InputType::kNumberInput:
      return isdigit(c);
    case InputType::kAnyInput:
    default:
      return isPrintable(c);
  }
}

/**
 * \brief Processes user input from the serial buffer and updates the output
 * string.
 *
 * This function handles character input, including special keys such as Enter,
 * Backspace, and Escape. It validates input characters based on the specified
 * `InputType` and appends them to an input buffer.
 *
 * \param type The expected input type (ALPHA_NUMERIC_INPUT, NUMERIC_INPUT,
 * etc.).
 * \param in_key The character received from serial input.
 * \param output Reference to a String where the processed input will be stored.
 * \return true if input is complete (Enter or Escape pressed), false otherwise.
 *
 * Functionality:
 * - Enter ('\r'): Finalizes input, stores it in `output`, clears the buffer,
 * and returns true.
 * - Escape (ASCII 27): Clears the buffer and returns true, canceling input.
 * - Backspace (ASCII 8) or Delete (ASCII 127): Removes the last character from
 * the buffer.
 * - Valid characters (determined by `isValidChar()`): Appends to buffer and
 * echoes to Serial.
 *
 * \note This function should be called continuously in a loop to handle user
 * input dynamically.
 */
bool ConfigManager::processInputBuffer(InputType type, char in_key,
                                       String &output) {
  if (in_key == '\r') {
    output = input_buffer_;
    input_buffer_ = "";
    Serial.println();
    return true;
  } else if (in_key == 27) {
    input_buffer_ = "";
    Serial.println();
    return true;
  } else if (in_key == 8 || in_key == 127) {
    if (input_buffer_.length() > 0) {
      input_buffer_.remove(input_buffer_.length() - 1);
      Serial.print("\b \b");
    }
  } else if (isValidChar(type, in_key)) {
    input_buffer_ += in_key;
    Serial.print(in_key);
  }

  return false;
}

/**
 * \brief Handles user input for a toggle (YES/NO) selection.
 *
 * This function reads a character input and updates a toggle choice.
 * It prints the current selection (YES or NO) to the serial output
 * and allows the user to toggle the choice using space or specific key presses.
 *
 * \param in_key The character input from the user.
 *              - '\r' (Enter) or 27 (ESC) confirms the selection.
 *              - 'Y'/'y' sets the choice to YES (1).
 *              - 'N'/'n' sets the choice to NO (0).
 *              - Space (' ') toggles the current selection.
 * \param group The bit in temp_group_data_ representing the group
 * \return true if the user confirms the selection (Enter or Escape pressed),
 * false otherwise.
 *
 * Functionality:
 * - Displays the current selection ("YES" or "NO") to the serial output.
 * - Allows users to toggle the selection with the space key.
 * - Accepts case-insensitive inputs for YES ('Y', 'y') and NO ('N', 'n').
 * - Confirms the selection when Enter or ESC is pressed and updates
 * return_choice.
 *
 * \note The function should be called repeatedly in a loop to allow real-time
 * user interaction.
 */
bool ConfigManager::getGroupDataInput(char in_key, uint8_t group) {
  Serial.print(temp_group_data_[group] ? "YES" : "NO ");
  Serial.print("\b\b\b");

  if (in_key == '\r' || in_key == 27) {  // Enter key
    Serial.println();
    return true;
  } else if (in_key == 32 || in_key == 'Y' || in_key == 'y' || in_key == 'N' ||
             in_key == 'n') {  // Accepted key strokes
    switch (in_key) {
      case 32:  // Space to toggle the selection.
        temp_group_data_[group].flip();
        break;
      case 'Y':
      case 'y':
        temp_group_data_[group] = 1;
        break;
      case 'N':
      case 'n':
        temp_group_data_[group] = 0;
        break;
    }

    Serial.print(temp_group_data_[group] ? "YES" : "NO ");
    Serial.print("\b\b\b");
  }

  return false;
}

/**
 * \brief Handles the process of adding a new contact through user input.
 *
 * This function guides the user through a step-by-step process to add a
 * contact, collecting details such as name, phone number, and group data
 * (Maintenance, Management, Statistics). It validates each input and ensures
 * uniqueness before adding the contact to the system.
 *
 * \param key The character received from serial input.
 * \return true if the process is ongoing, false when the contact is
 * successfully added or an error occurs.
 *
 * Functionality:
 * - Ensures the maximum number of contacts is not exceeded.
 * - Guides the user through the following input steps:
 *   1. **Full Name:** Must be valid and unique.
 *   2. **Phone Number:** Must be valid and unique.
 *   4. **Group Data:** Toggles for Maintenance, Management, and Statistics.
 * - Stores contact data upon successful entry.
 *
 * State Transitions:
 * - `INIT` -> `NAME_INPUT` -> `CONTACT_INPUT` -> `LOCATION_INPUT` ->
 *   `GROUP_DATA_MAINTEANCE_INPUT` -> `GROUP_DATA_MANAGEMENT_INPUT` ->
 * `GROUP_DATA_STATISTICS_INPUT`
 * - Upon completion, the contact is stored, and the function exits.
 *
 * \note This function should be called continuously to handle interactive input
 * processing.
 */
bool ConfigManager::addContact(char key) {
  menu_state_ = MenuState::kAddContact;

  if (person_manager_.getCount() >= Person::kMaxEntries) {
    Serial.println("Maximum number of contacts reached.");
    return false;
  }

  switch (contact_menu_state_) {
    case ContactState::kInit:
      Serial.print("Full name: ");
      contact_menu_state_ = ContactState::kNameInput;
      return true;

    case ContactState::kNameInput:
      if (processInputBuffer(InputType::kAlphaNumericInput, key, temp_name_)) {
        if (temp_name_.isEmpty()) return false;
        if (!PersonManager::isValidName(temp_name_)) {
          Serial.println("Invalid name!");
          return false;
        }
        if (person_manager_.search(temp_name_)) {
          Serial.println("Name already exists.");
          return false;
        }
        Serial.print("Phone number: ");
        contact_menu_state_ = ContactState::kPhoneNumberInput;
      }
      return true;

    case ContactState::kPhoneNumberInput:
      if (processInputBuffer(InputType::kPhoneNumInput, key, temp_contact_)) {
        if (temp_contact_.isEmpty()) return false;
        if (!PersonManager::isValidPhoneNumber(temp_contact_)) {
          Serial.println("Invalid phone number!");
          return false;
        }
        if (person_manager_.isNumberExists("", temp_contact_)) {
          Serial.println("Phone number already exists.");
          return false;
        }
        Serial.print("Maintenance: ");
        contact_menu_state_ = ContactState::kGroupDataMaintenanceInput;
        getGroupDataInput(0, kGroupDataMaintenanceBit);
      }
      return true;

    case ContactState::kGroupDataMaintenanceInput:
      if (getGroupDataInput(key, kGroupDataMaintenanceBit)) {
        Serial.print("Management: ");
        contact_menu_state_ = ContactState::kGroupDataManagementInput;
        getGroupDataInput(0, kGroupDataManagementBit);
      }
      return true;

    case ContactState::kGroupDataManagementInput:
      if (getGroupDataInput(key, kGroupDataMaintenanceBit)) {
        Serial.print("Statistics: ");
        contact_menu_state_ = ContactState::kGroupDataStatisticsInput;
        getGroupDataInput(0, kGroupDataStatisticsBit);
      }
      return true;

    case ContactState::kGroupDataStatisticsInput:
      if (getGroupDataInput(key, kGroupDataStatisticsBit)) {
        person_manager_.add(temp_name_, temp_contact_, temp_group_data_);
        saveConfig();
        Serial.println("Contact added successfully.");
        return false;
      }
      return true;
  }

  return false;
}

/**
 * \brief Clears the contact name from the serial console in edit mode.
 *
 * This function removes the displayed contact name by printing backspace
 * ('\b')s for each character in the string. This effectively erases the name
 * from the serial output.
 *
 * \param name The contact name to be cleared from the serial console.
 *
 * Functionality:
 * - Iterates through each character in the provided name.
 * - Uses backspace sequences to overwrite the existing text.
 *
 * \note This function only affects the visual output on the serial monitor.
 *       It does not modify the actual string content.
 */
void ConfigManager::clearContactNameInEditMode(String &name) {
  for (int i = 0; i < name.length(); i++) {
    Serial.print("\b \b");
  }
}

/**
 * \brief Handles user input for selecting a contact name from a list.
 *
 * This function processes user input to navigate through a list of contact
 * names (using '<' or to go up and '>' to go down) and displays the selected
 * name. The function allows the user to select a name (Enter) or cancel the
 * selection (Escape).
 *
 * \param key The character received from serial input for navigation or
 * selection.
 * \param name The current contact name displayed, which gets updated
 * with the selected name.
 * \return SELECTION_TYPE The state of the selection process:
 *         - `SELECTING`: The user is still selecting.
 *         - `SELECTED`: The user has confirmed their selection (Enter pressed).
 *         - `NOT_SELECTED`: The user has canceled their selection (Escape
 * pressed).
 *
 * Functionality:
 * - Navigation: The user can move through the list of names with '<' (or ',')
 * to move up, and '>' (or '.') to move down.
 * - Display: The name at the selected position is displayed after each
 * navigation.
 * - Selection: Pressing Enter confirms the selection, while Escape cancels it.
 *
 * \note This function modifies the `name` to reflect the current selection
 * and uses `clearContactNameInEditMode()` to remove the old name before
 * displaying the new one.
 */
ConfigManager::SelectionType ConfigManager::nameSelection(char key,
                                                          String &name) {
  if (key == '<' || key == ',') {
    selected_buffer_ = (selected_buffer_ - 1);
    if (selected_buffer_ < 0) {
      selected_buffer_ = person_manager_.getCount() - 1;
    }

    clearContactNameInEditMode(name);
    if (!person_manager_.getNameAtPosition(selected_buffer_, name)) {
      return SelectionType::kNotSelected;
    }
    Serial.print(name);
  } else if (key == '>' || key == '.') {
    selected_buffer_ = (selected_buffer_ + 1) % person_manager_.getCount();

    clearContactNameInEditMode(name);
    if (!person_manager_.getNameAtPosition(selected_buffer_, name)) {
      return SelectionType::kNotSelected;
    }
    Serial.print(name);
  } else if (key == '\r') {
    return SelectionType::kSelected;
  } else if (key == 27) {
    return SelectionType::kNotSelected;
  }

  return SelectionType::kSelecting;
}

/**
 * \brief Handles the process of editing an existing contact's details.
 *
 * This function allows the user to navigate through a list of contacts, select
 * one to edit, and then update details such as name, phone number, and group
 * data (Maintenance, Management, Statistics). It performs input validation and
 * ensures data consistency during the editing process.
 *
 * \param key The character received from serial input to navigate or edit
 * fields.
 * \return true if the process is ongoing and further user input is required,
 * false when the editing process is complete or an error occurs.
 *
 * Functionality:
 * - Navigation: The user selects a contact by navigating the list using the
 * '<' or '>' keys.
 * - Name Editing: The user edits the contact's name, ensuring it’s valid and
 * unique.
 * - Contact Editing: The user updates the phone number, ensuring it’s valid
 * and unique.
 * - Group Data Editing: The user toggles settings for Maintenance, Management,
 * and Statistics.
 * - Final Update: Upon completing the editing, the contact’s data is saved
 * and updated.
 *
 * State Transitions:
 * - `INIT` -> `NAME_SELECT` -> `NAME_INPUT` -> `CONTACT_INPUT` ->
 * `GROUP_DATA_MAINTEANCE_INPUT` -> `GROUP_DATA_MANAGEMENT_INPUT` ->
 * `GROUP_DATA_STATISTICS_INPUT`
 * - Upon completion, the contact’s information is updated, and the function
 * exits.
 *
 * \note This function ensures the contact's data is validated and only
 * updated if all fields are correctly filled.
 */
bool ConfigManager::editContact(char key) {
  menu_state_ = MenuState::kEditContact;

  if (person_manager_.getCount() == 0) {
    Serial.println("No contacts to edit.");
    return false;
  }

  switch (contact_menu_state_) {
    case ContactState::kInit:
      Serial.println(
          "Select the name to edit from the list below. Use < and > keys to "
          "navigate.\r\n");
      Serial.print("Full name: ");
      if (!person_manager_.getNameAtPosition(selected_buffer_,
                                             selected_name_buffer)) {
        return false;
      }
      Serial.print(selected_name_buffer);
      contact_menu_state_ = ContactState::kNameSelect;
      return true;

    case ContactState::kNameSelect: {
      SelectionType selectionType = nameSelection(key, selected_name_buffer);

      if (selectionType == SelectionType::kSelecting) {
        return true;
      } else if (selectionType == SelectionType::kSelected) {
        Serial.println(
            "\r\n\nPress any key to edit the selected field or press Enter or "
            "Esc to skip the field.\r\n");

        const Person *person = person_manager_.search(selected_name_buffer);
        if (person == nullptr) {
          Serial.println("Contact details are not found.");
          return false;
        }

        selected_contact_.phone_number = person->phone_number;
        selected_contact_.group_data = person->group_data;
        selected_contact_.name = person->name;

        temp_contact_ = person->phone_number;
        temp_name_ = person->name;
        temp_group_data_ = person->group_data;

        Serial.print("Full name: ");
        selection_to_edit_ = temp_name_;
        Serial.print(selection_to_edit_);

        edit_mode_ = EditState::kEditInit;
        contact_menu_state_ = ContactState::kNameInput;
        return true;
      }
    }

    case ContactState::kNameInput: {
      if (!editContactField(selection_to_edit_, InputType::kAlphaNumericInput,
                            key)) {
        if (selection_to_edit_.isEmpty()) {
          return false;
        }

        if (PersonManager::isValidName(selection_to_edit_) == false) {
          Serial.println("Invalid name!");
          return false;
        }

        if (person_manager_.isDuplicateEntryExists(temp_name_,
                                                   selection_to_edit_)) {
          Serial.println("Name already exists.");
          return false;
        }

        temp_name_ = selection_to_edit_;

        Serial.print("Phone number: ");
        selection_to_edit_ = temp_contact_;
        Serial.print(selection_to_edit_);

        edit_mode_ = EditState::kEditInit;
        contact_menu_state_ = ContactState::kPhoneNumberInput;
      }

      return true;
    }
    case ContactState::kPhoneNumberInput: {
      if (!editContactField(selection_to_edit_, InputType::kPhoneNumInput,
                            key)) {
        if (selection_to_edit_.isEmpty()) {
          return false;
        }

        if (PersonManager::isValidPhoneNumber(selection_to_edit_) == false) {
          Serial.println("Invalid phone number!");
          return false;
        }

        if (person_manager_.isNumberExists(temp_contact_, selection_to_edit_)) {
          Serial.println("Phone number already exists.");
          return false;
        }

        temp_contact_ = selection_to_edit_;

        Serial.print("Maintenance: ");

        edit_mode_ = EditState::kEditInit;
        contact_menu_state_ = ContactState::kGroupDataMaintenanceInput;

        getGroupDataInput(0, kGroupDataMaintenanceBit);
      }

      return true;
    }
    case ContactState::kGroupDataMaintenanceInput: {
      if (getGroupDataInput(key, kGroupDataMaintenanceBit)) {
        Serial.print("Management: ");

        edit_mode_ = EditState::kEditInit;
        contact_menu_state_ = ContactState::kGroupDataManagementInput;

        getGroupDataInput(0, kGroupDataManagementBit);
      }

      return true;
    }
    case ContactState::kGroupDataManagementInput: {
      if (getGroupDataInput(key, kGroupDataManagementBit)) {
        Serial.print("Statistics: ");

        edit_mode_ = EditState::kEditInit;
        contact_menu_state_ = ContactState::kGroupDataStatisticsInput;

        getGroupDataInput(0, kGroupDataStatisticsBit);
      }

      return true;
    }
    case ContactState::kGroupDataStatisticsInput: {
      if (getGroupDataInput(key, kGroupDataStatisticsBit)) {
        person_manager_.edit(selected_contact_.name, temp_name_, temp_contact_,
                             temp_group_data_);
        saveConfig();
        Serial.println("Contact edited successfully.");
        return false;
      }

      return true;
    }
  }

  return false;
}

/**
 * \brief Manages the editing of a contact's field (name, phone number, groups).
 *
 * This function allows the user to edit a specific contact field by handling
 * the input character, processing it, and managing the edit state. The user can
 * cancel the edit process by pressing Enter or Escape, or they can modify the
 * field in edit mode.
 *
 * \param contact_data The contact data to be edited. This will be updated
 * with new input.
 * \param type The expected input type (ALPHA_NUMERIC_INPUT, NUMERIC_INPUT,
 * etc.).
 * \param in_key The character received from serial input.
 * \return true if the user is still in the editing mode and further input is
 * needed, false if the editing process is complete or canceled.
 *
 * Functionality:
 * - If the user is not in editing mode and presses Enter or Escape, the edit
 * process is canceled.
 * - If in the initialization state (`EDIT_INIT`), the user’s input is accepted,
 * and the field is cleared for editing.
 * - If in the editing state (`EDITING`), the input is processed, and the field
 * is updated.
 * - If editing is complete (Enter or Escape pressed), the edit state is reset
 * to `EDIT_INIT`.
 *
 * \note The function handles input validation and clears the contact's data
 * when transitioning into editing mode.
 */
bool ConfigManager::editContactField(String &contact_data, InputType type,
                                     char in_key) {
  if ((edit_mode_ != EditState::kEditing) && (in_key == '\r' || in_key == 27)) {
    Serial.println();
    edit_mode_ = EditState::kEditInit;
    return false;
  }

  if (edit_mode_ == EditState::kEditInit && isValidChar(type, in_key)) {
    clearContactNameInEditMode(contact_data);

    edit_mode_ = EditState::kEditing;

    contact_data = "";
    input_buffer_ = in_key;

    Serial.print(in_key);
    return true;
  }

  if (edit_mode_ == EditState::kEditing) {
    if (processInputBuffer(type, in_key, contact_data)) {
      edit_mode_ = EditState::kEditInit;
      return false;
    }

    return true;
  }

  return false;
}

const std::vector<Person> &ConfigManager::getAllContacts() const {
  return person_manager_.getAllPeople();
}

const String &ConfigManager::getLocation() const { return location_; }

bool ConfigManager::handleImprovUserData(const JsonObjectConst &data) {
  JsonObjectConst config = data["config"].as<JsonObjectConst>();
  if (config.isNull() || config.size() == 0) {
    TRACELN("No config");
    return true;
  }
  parseConfig(config);
  saveConfig();
  return true;
}

/**
 * \brief Displays the detailed information of a contact
 *
 * This function prints the contact's full name, phone number, and group data
 * (Maintenance, Management, and Statistics) to the provided serial display.
 *
 * \param person The `Person` structure containing the contact's
 * information to be displayed.
 *
 * Functionality:
 * - Displays the contact's **Full Name** and **Phone Number**
 * - Displays the **Maintenance**, **Management**, and **Statistics** group data
 * as either "YES" or "NO" based on bit flags.
 * - Separates the data with a dashed line for clarity.
 */
void ConfigManager::printPerson(const Person &person) {
  Serial.println("Full name: " + person.name);
  Serial.println("Contact: " + person.phone_number);

  Serial.print("Maintenance: ");
  Serial.println((person.group_data[kGroupDataMaintenanceBit]) ? "YES" : "NO");

  Serial.print("Management: ");
  Serial.println((person.group_data[kGroupDataManagementBit]) ? "YES" : "NO");

  Serial.print("Statistics: ");
  Serial.println((person.group_data[kGroupDataStatisticsBit]) ? "YES" : "NO");

  Serial.println("-------------------");
}

/**
 * \brief Deletes a contact from the list based on user selection.
 *
 * This function allows the user to select a contact to delete from the list.
 * The user can navigate through the list using the '<' and '>' keys, select a
 * contact, and then confirm deletion. If no contacts are available, a
 * message is shown.
 *
 * \param key The character received from serial input to navigate or confirm
 * the deletion.
 * \return true if the user is still in the selection process and further input
 * is required, false when the deletion process is completed or canceled.
 *
 * Functionality:
 * - Navigation: The user selects a contact to delete using the '<' or '>'
 * keys.
 * - Deletion: Upon selection, the contact’s information is deleted from the
 * list if found.
 * - Confirmation: Success or failure messages are shown after attempting to
 * delete the contact.
 *
 * \note This function interacts with the `PersonManager` to delete the selected
 * contact’s data.
 */
bool ConfigManager::deleteContact(char key) {
  menu_state_ = MenuState::kDeleteContact;

  if (person_manager_.getCount() == 0) {
    Serial.println("No contacts to delete.");
    return false;
  }

  if (contact_menu_state_ == ContactState::kInit) {
    Serial.println(
        "Select the name to delete from the list below. Use < and > keys to "
        "navigate.\r\n");
    Serial.print("Full name: ");
    if (!person_manager_.getNameAtPosition(selected_buffer_,
                                           selected_name_buffer)) {
      return false;
    }
    Serial.print(selected_name_buffer);
    contact_menu_state_ = ContactState::kNameSelect;
    return true;
  }

  if (contact_menu_state_ == ContactState::kNameSelect) {
    SelectionType selectionType = nameSelection(key, selected_name_buffer);

    if (selectionType == SelectionType::kSelecting) {
      return true;
    } else if (selectionType == SelectionType::kSelected) {
      if (selected_name_buffer.isEmpty()) {
        return false;
      }

      Serial.println();

      if (person_manager_.remove(selected_name_buffer) == 0) {
        Serial.println("Failed to delete contact.");
      } else {
        saveConfig();
        Serial.println("Contact deleted successfully.");
      }
    }
  }

  return false;
}

/**
 * \brief Displays a list of all contacts.
 *
 * This function displays all the contacts in the system. If no contacts are
 * available, it will show a message indicating that there are no contacts to
 * display. If contacts exist, their details are printed using the
 * `printPerson` function.
 *
 * \note This function relies on `person_manager_` to retrieve and print the
 * contacts' details.
 */
void ConfigManager::showAllContacts() {
  if (person_manager_.getCount() == 0) {
    Serial.println("No contacts to show.");
    return;
  }

  person_manager_.printAll(printPerson);
}

/**
 * \brief Prints the main configuration menu to the serial display.
 *
 * This function displays a menu with options for managing contact data. The
 * available options include viewing the contact list, adding, editing,
 * deleting and exiting the configuration menu.
 *
 * \note The `menu_state_` is updated to `MAIN_MENU` upon entering this
 * function, and the options are displayed to guide the user in making a
 * selection.
 */
void ConfigManager::printMenu() {
  menu_state_ = MenuState::kMainMenu;

  Serial.println("\r\nDevice Configuration Menu\r\n");
  Serial.printf("[1] Show contact list (%d)\n", person_manager_.getCount());
  Serial.println("[2] Add contact");
  Serial.println("[3] Edit contact details");
  Serial.println("[4] Remove contact");
#ifdef RTC_MANAGER
  const String time = TimeManager::getFormattedTime();
  Serial.printf("[5] Set system date/time (%s)\n", time.c_str());
#endif
  Serial.println("[6] Show log entries");
  Serial.printf("[7] Edit location name (%s)\n", location_.c_str());
  Serial.println("[X] Exit configuration menu");
  Serial.print("\r\nSelection: ");
}

#ifdef RTC_MANAGER
/**
 * \brief Sets the system date and time based on user input.
 *
 * This function allows the user to set the system date and time by entering
 * values for year, month, day, hour, minute, and second. It validates each
 * input and updates the system time accordingly.
 *
 * \param key The character received from serial input for date/time entry.
 * \return true if the process is ongoing, false when the date/time is set or
 * an error occurs.
 *
 * Functionality:
 * - Guides the user through entering year, month, day, hour, minute, and
 * second.
 * - Validates each input to ensure correctness.
 * - Sets the system time using `TimeManager::setSystemTime`.
 *
 * \note This function interacts with `TimeManager` to set the system date and
 * time.
 */
bool ConfigManager::setSystemDateTime(char key) {
  menu_state_ = MenuState::kDateTimeSet;

  switch (date_input_stage) {
    case SetDateTimeStage::SDT_INIT:
      Serial.print("Year: ");
      date_input_stage = SetDateTimeStage::SDT_YEAR;
      break;
    case SetDateTimeStage::SDT_YEAR:
      if (processInputBuffer(InputType::kNumberInput, key, temp_number_)) {
        if (temp_number_.isEmpty()) return false;

        if (!TimeManager::isValidYear(temp_number_)) {
          Serial.println("Invalid year!");
          return false;
        }

        input_year_ = temp_number_.toInt();

        date_input_stage = SetDateTimeStage::SDT_MONTH;
        temp_number_ = "";
        Serial.print("Month: ");

        return true;
      }
      break;

    case SetDateTimeStage::SDT_MONTH:
      if (processInputBuffer(InputType::kNumberInput, key, temp_number_)) {
        if (temp_number_.isEmpty()) return false;

        if (!TimeManager::isValidMonth(temp_number_)) {
          Serial.println("Invalid month!");
          return false;
        }

        input_month_ = temp_number_.toInt();

        date_input_stage = SetDateTimeStage::SDT_DAY;
        temp_number_ = "";
        Serial.print("Day: ");

        return true;
      }
      break;

    case SetDateTimeStage::SDT_DAY:
      if (processInputBuffer(InputType::kNumberInput, key, temp_number_)) {
        if (temp_number_.isEmpty()) return false;

        if (!TimeManager::isValidDay(temp_number_, input_year_, input_month_)) {
          Serial.println("Invalid day!");
          return false;
        }

        input_day_ = temp_number_.toInt();

        date_input_stage = SetDateTimeStage::SDT_HOUR;
        temp_number_ = "";
        Serial.print("Hour: ");

        return true;
      }
      break;
    case SetDateTimeStage::SDT_HOUR:
      if (processInputBuffer(InputType::kNumberInput, key, temp_number_)) {
        if (temp_number_.isEmpty()) return false;

        if (!TimeManager::isValidHour(temp_number_)) {
          Serial.println("Invalid hour!");
          return false;
        }

        input_hour_ = temp_number_.toInt();

        date_input_stage = SetDateTimeStage::SDT_MINUTE;
        temp_number_ = "";
        Serial.print("Minute: ");

        return true;
      }
      break;
    case SetDateTimeStage::SDT_MINUTE:
      if (processInputBuffer(InputType::kNumberInput, key, temp_number_)) {
        if (temp_number_.isEmpty()) return false;

        if (!TimeManager::isValidMinute(temp_number_)) {
          Serial.println("Invalid minute!");
          return false;
        }

        input_minute_ = temp_number_.toInt();

        date_input_stage = SetDateTimeStage::SDT_SECOND;
        temp_number_ = "";
        Serial.print("Second: ");

        return true;
      }
      break;
    case SetDateTimeStage::SDT_SECOND:
      if (processInputBuffer(InputType::kNumberInput, key, temp_number_)) {
        if (temp_number_.isEmpty()) return false;

        // Check if the second is valid (using the same function as minute).
        if (!TimeManager::isValidMinute(temp_number_)) {
          Serial.println("Invalid second!");
          return false;
        }

        input_second_ = temp_number_.toInt();

        TimeManager::setSystemTime(input_year_, input_month_, input_day_,
                                   input_hour_, input_minute_, input_second_);

        date_input_stage = SetDateTimeStage::SDT_INIT;
        temp_number_ = "";
        Serial.println("Date/Time set successfully.");
        return false;
      }
      break;
  }

  return true;
}
#endif

/**
 * \brief Edits the location name based on user input.
 *
 * This function allows the user to edit the location name by entering a new
 * name. It validates the input and updates the location name if valid. The
 * function handles the input state and provides feedback to the user.
 *
 * \param key The character received from serial input for editing the location
 * name.
 * \return true if the process is ongoing, false when the location name is set
 * or an error occurs.
 *
 */
bool ConfigManager::editLocationName(char key) {
  menu_state_ = MenuState::kEditLocationName;

  if (location_edit_state == LocationEditState::kLocationInit) {
    Serial.print("Enter the location name: ");
    location_edit_state = LocationEditState::kLocationInput;
    return true;
  } else {
    if (processInputBuffer(InputType::kAnyInput, key, temp_location_)) {
      if (isValidLocation(temp_location_)) {
        location_ = temp_location_;
        saveConfig();
      } else {
        return false;
      }

      location_edit_state = LocationEditState::kLocationInit;
      return false;
    }
  }

  return true;
}

/**
 * \brief Resets the internal states and buffers to their initial values.
 *
 * This function resets all relevant variables and states used in the contact
 * management process to their default or initial values. It ensures that any
 * data entered or selected during previous operations is cleared, effectively
 * resetting the configuration state.
 *
 * \note This function is useful for starting fresh or after completing certain
 * actions like adding, editing, or deleting contacts, ensuring no residual
 * data interferes with future operations.
 */
void ConfigManager::resetSubState() {
  contact_menu_state_ = ContactState::kInit;
  input_buffer_ = "";
  selected_name_buffer = "";
  selected_buffer_ = 0;
  selection_to_edit_ = "";
  edit_mode_ = EditState::kEditInit;

  temp_name_ = "";
  temp_contact_ = "";
  temp_group_data_.reset();
  temp_location_ = "";

  selected_contact_.phone_number = "";
  selected_contact_.group_data = 0;
  selected_contact_.name = "";

  date_input_stage = SetDateTimeStage::SDT_INIT;
  location_edit_state = LocationEditState::kLocationInit;
}

/**
 * \brief Handles user input and processes menu options.
 *
 * This function processes a single user input, performs the corresponding
 * action based on the menu selection, and updates the menu state accordingly.
 * It supports operations like showing all contacts, adding, editing,
 * deleting and exiting the configuration menu.
 *
 * \param input The character representing the user's menu selection.
 *
 * \note After processing a valid input, the menu state is updated, and the main
 * menu is reprinted unless the operation is still in progress (e.g., adding or
 * editing a contact).
 */
void ConfigManager::handleInput(char input) {
  bool is_still_in_state = false;
  resetSubState();

  switch (input) {
    case '1':
      showAllContacts();
      break;
    case '2':
      is_still_in_state = addContact(input);
      break;
    case '3':
      is_still_in_state = editContact(input);
      break;
    case '4':
      is_still_in_state = deleteContact(input);
      break;
#ifdef RTC_MANAGER
    case '5':
      is_still_in_state = setSystemDateTime(input);
      break;
#endif
    case '6':
      LoggingManager::showAllLogs();
      break;
    case '7':
      is_still_in_state = editLocationName(input);
      break;
    case 'x':
    case 'X':
      Serial.println("Exiting configuration menu.");
      menu_state_ = MenuState::kIdle;
      return;
    default:
      Serial.println("Invalid option.");
      break;
  }

  if (!is_still_in_state) {
    printMenu();
  }
}

/**
 * \brief Parses a JSON object with location and contacts
 *
 * Expects a JSON in the form of {"location": "...", "people": [...]}
 */
void ConfigManager::parseConfig(JsonObjectConst config) {
  location_ = config["location"].as<String>();
  person_manager_.clear();
  person_manager_.parseJson(config["people"].as<JsonArrayConst>());
}

void ConfigManager::saveConfig() {
  JsonDocument doc;
  JsonObject config = doc.to<JsonObject>();
  config["location"] = location_;
  person_manager_.serializeJson(config["people"].to<JsonArray>());
  storage_->storeCustomConfig(config);
}

/**
 * \brief Validates if a given location name is valid based on specified
 * criteria.
 *
 * This function checks if the provided location name contains only valid
 * characters: alphabets, spaces, hyphens, and apostrophes. The name must also
 * not be empty. This function is useful for ensuring that location names
 * conform to typical formats (e.g., "New York", "St. John's").
 *
 * \param location The location name string to be validated.
 * \return `true` if the location name contains only valid characters and is not
 * empty, `false` otherwise.
 */
bool ConfigManager::isValidLocation(const String &location) {
  for (size_t i = 0; i < location.length(); i++) {
    char c = location[i];
    if (!(isAlpha(c) || c == ' ' || c == '-' || c == '\'' || c == '.')) {
      return false;
    }
  }
  if (location.length() > kMaxLocationLength) {
    Serial.printf("Location max length is %d\n", kMaxLocationLength);
    return false;
  }
  return !location.isEmpty();
}

const uint8_t ConfigManager::kMaxLocationLength = 30;

}  // namespace inamata
