#include "config_menu.h"

#include <Arduino.h>

#include "managers/time_manager.h"
#include "managers/log_manager.h"
#include "utils/person_info.h"
#include "utils/person_info_validator.h"

namespace inamata {

void ConfigManager::initConfigMenuManager() {
  resetSubState();
  menuState = MenuState::kIdle;

  Serial.println("Press [C] to enter configuration menu.");
  return;
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
 * - In other states (ADD, EDIT, DELETE, SEARCH recipient), it handles the
 * respective actions.
 * - If a stateful operation completes, the menu is reprinted for user guidance.
 *
 * \note This function should be called repeatedly to ensure continuous
 * interaction.
 */
void ConfigManager::loop() {
  bool isStillInState = false;

  if (Serial.available()) {
    char key = Serial.read();

    if (menuState == MenuState::kIdle) {
      if ((key == 'C') || (key == 'c')) {  // 'C' key enters config mode
        printMenu();
        return;
      }
    } else if (menuState == MenuState::kMainMenu) {
      if (!(key == '\r' || key == '\n')) {
        Serial.print(key);
        Serial.println("\r\n");
        handleInput(key);
      }

      return;
    } else if (menuState == MenuState::kAddRecipient) {
      isStillInState = addRecipient(key);
    } else if (menuState == MenuState::kEditRecipient) {
      isStillInState = editRecipient(key);
    } else if (menuState == MenuState::kDeleteRecipient) {
      isStillInState = deleteRecipient(key);
    } else if (menuState == MenuState::kSearchRecipient) {
      isStillInState = searchRecipient(key);
#ifdef RTC_MANAGER      
    } else if (menuState == MenuState::kDateTimeSet) {
      isStillInState = setSystemDateTime(key);
#endif
    }
    // Show main menu if state machine is terminated or in idle mode.
    if (!isStillInState) {
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
      return isdigit(c) || c == '+';
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
 * \param inKey The character received from serial input.
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
bool ConfigManager::processInputBuffer(InputType type, char inKey,
                                       String &output) {
  if (inKey == '\r') {
    output = inputBuffer;
    inputBuffer = "";
    Serial.println();
    return true;
  } else if (inKey == 27) {
    inputBuffer = "";
    Serial.println();
    return true;
  } else if (inKey == 8 || inKey == 127) {
    if (inputBuffer.length() > 0) {
      inputBuffer.remove(inputBuffer.length() - 1);
      Serial.print("\b \b");
    }
  } else if (isValidChar(type, inKey)) {
    inputBuffer += inKey;
    Serial.print(inKey);
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
 * \param inKey The character input from the user.
 *              - '\r' (Enter) or 27 (ESC) confirms the selection.
 *              - 'Y'/'y' sets the choice to YES (1).
 *              - 'N'/'n' sets the choice to NO (0).
 *              - Space (' ') toggles the current selection.
 * \param returnChoice Pointer to a char where the final selection will be
 * stored.
 * \return true if the user confirms the selection (Enter or Escape pressed),
 * false otherwise.
 *
 * Functionality:
 * - Displays the current selection ("YES" or "NO") to the serial output.
 * - Allows users to toggle the selection with the space key.
 * - Accepts case-insensitive inputs for YES ('Y', 'y') and NO ('N', 'n').
 * - Confirms the selection when Enter or ESC is pressed and updates
 * returnChoice.
 *
 * \note The function should be called repeatedly in a loop to allow real-time
 * user interaction.
 */
bool ConfigManager::getToggleInput(char inKey, char *returnChoice) {
  Serial.print(choiceBuffer ? "YES" : "NO ");
  Serial.print("\b\b\b");

  if (inKey == '\r' || inKey == 27) {  // Enter key
    Serial.println();
    *returnChoice = choiceBuffer;
    return true;
  } else if (inKey == 32 || inKey == 'Y' || inKey == 'y' || inKey == 'N' ||
             inKey == 'n') {  // Accepted key strokes
    switch (inKey) {
      case 32:  // Space to toggle the selection.
        choiceBuffer = !choiceBuffer;
        break;
      case 'Y':
      case 'y':
        choiceBuffer = 1;
        break;
      case 'N':
      case 'n':
        choiceBuffer = 0;
        break;
    }

    Serial.print(choiceBuffer ? "YES" : "NO ");
    Serial.print("\b\b\b");
  }

  return false;
}

/**
 * \brief Handles the process of adding a new recipient through user input.
 *
 * This function guides the user through a step-by-step process to add a
 * recipient, collecting details such as name, contact number, site name, and
 * group data (Maintenance, Management, Statistics). It validates each input and
 * ensures uniqueness before adding the recipient to the system.
 *
 * \param key The character received from serial input.
 * \return true if the process is ongoing, false when the recipient is
 * successfully added or an error occurs.
 *
 * Functionality:
 * - Ensures the maximum number of recipients is not exceeded.
 * - Guides the user through the following input steps:
 *   1. **Full Name:** Must be valid and unique.
 *   2. **Contact Number:** Must be valid and unique.
 *   3. **Site Name:** Any non-empty alphanumeric input.
 *   4. **Group Data:** Toggles for Maintenance, Management, and Statistics.
 * - Stores recipient data upon successful entry.
 *
 * State Transitions:
 * - `INIT` -> `NAME_INPUT` -> `CONTACT_INPUT` -> `SITE_INPUT` ->
 *   `GROUP_DATA_MAINTEANCE_INPUT` -> `GROUP_DATA_MANAGEMENT_INPUT` ->
 * `GROUP_DATA_STATISTICS_INPUT`
 * - Upon completion, the recipient is stored, and the function exits.
 *
 * \note This function should be called continuously to handle interactive input
 * processing.
 */
bool ConfigManager::addRecipient(char key) {
  menuState = MenuState::kAddRecipient;

  if (infoManager.getPersonalInfoCount() >= MAX_PERSONAL_INFO_COUNT) {
    Serial.println("Maximum number of recipients reached.");
    return false;
  }

  switch (recipientMenuState) {
    case RecipientState::kInit:
      Serial.print("Full name: ");
      recipientMenuState = RecipientState::kNameInput;
      return true;

    case RecipientState::kNameInput:
      if (processInputBuffer(InputType::kAlphaNumericInput, key, tempName)) {
        if (tempName.isEmpty()) return false;
        if (!Validator::isValidName(tempName)) {
          Serial.println("Invalid name!");
          return false;
        }
        if (infoManager.searchInfo(tempName)) {
          Serial.println("Name already exists.");
          return false;
        }
        Serial.print("Contact number: ");
        recipientMenuState = RecipientState::kContactInput;
      }
      return true;

    case RecipientState::kContactInput:
      if (processInputBuffer(InputType::kPhoneNumInput, key, tempContact)) {
        if (tempContact.isEmpty()) return false;
        if (!Validator::isValidPhoneNumber(tempContact)) {
          Serial.println("Invalid contact number!");
          return false;
        }
        if (infoManager.isNumberExists("", tempContact)) {
          Serial.println("Contact number already exists.");
          return false;
        }
        Serial.print("Site name: ");
        recipientMenuState = RecipientState::kSiteInput;
      }
      return true;

    case RecipientState::kSiteInput:
      if (processInputBuffer(InputType::kAlphaNumericInput, key, tempSite)) {
        if (tempSite.isEmpty()) return false;
        Serial.print("Maintenance: ");
        recipientMenuState = RecipientState::kGroupDataMaintenanceInput;
        choiceBuffer = 0;
        getToggleInput(0, &tempMaintanence);
      }
      return true;

    case RecipientState::kGroupDataMaintenanceInput:
      if (getToggleInput(key, &tempMaintanence)) {
        Serial.print("Management: ");
        recipientMenuState = RecipientState::kGroupDataManagementInput;
        choiceBuffer = 0;
        getToggleInput(0, &tempManagement);
      }
      return true;

    case RecipientState::kGroupDataManagementInput:
      if (getToggleInput(key, &tempManagement)) {
        Serial.print("Statistics: ");
        recipientMenuState = RecipientState::kGroupDataStatisticsInput;
        choiceBuffer = 0;
        getToggleInput(0, &tempStatistics);
      }
      return true;

    case RecipientState::kGroupDataStatisticsInput:
      if (getToggleInput(key, &tempStatistics)) {
        char groupData = (tempMaintanence ? 0x01 : 0x00) |
                         (tempManagement ? 0x02 : 0x00) |
                         (tempStatistics ? 0x04 : 0x00);

        infoManager.addInfo(tempName, tempContact, tempSite, groupData);
        Serial.println("Recipient added successfully.");
        return false;
      }
      return true;
  }

  return false;
}

/**
 * \brief Clears the recipient name from the serial console in edit mode.
 *
 * This function removes the displayed recipient name by printing backspace
 * ('\b')s for each character in the string. This effectively erases the name
 * from the serial output.
 *
 * \param name The recipient name to be cleared from the serial console.
 *
 * Functionality:
 * - Iterates through each character in the provided name.
 * - Uses backspace sequences to overwrite the existing text.
 *
 * \note This function only affects the visual output on the serial monitor.
 *       It does not modify the actual string content.
 */
void ConfigManager::clearRecepientNameInEditMode(String &name) {
  for (int i = 0; i < name.length(); i++) {
    Serial.print("\b \b");
  }
}

/**
 * \brief Handles user input for selecting a recipient name from a list.
 *
 * This function processes user input to navigate through a list of recipient
 * names (using '<' or to go up and '>' to go down) and displays the selected
 * name. The function allows the user to select a name (Enter) or cancel the
 * selection (Escape).
 *
 * \param key The character received from serial input for navigation or
 * selection.
 * \param lastName The current recipient name displayed, which gets updated with
 * the selected name.
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
 * \note This function modifies the `lastName` to reflect the current selection
 * and uses `clearRecepientNameInEditMode()` to remove the old name before
 * displaying the new one.
 */
ConfigManager::SelectionType ConfigManager::nameSelection(char key,
                                                          String &lastName) {
  if (key == '<' || key == ',') {
    selectedBuffer = (selectedBuffer - 1);
    if (selectedBuffer < 0) {
      selectedBuffer = infoManager.getPersonalInfoCount() - 1;
    }

    clearRecepientNameInEditMode(lastName);
    if (!infoManager.getNameAtPosition(selectedBuffer, lastName)) {
      return SelectionType::kNotSelected;
    }
    Serial.print(lastName);
  } else if (key == '>' || key == '.') {
    selectedBuffer = (selectedBuffer + 1) % infoManager.getPersonalInfoCount();

    clearRecepientNameInEditMode(lastName);
    if (!infoManager.getNameAtPosition(selectedBuffer, lastName)) {
      return SelectionType::kNotSelected;
    }
    Serial.print(lastName);
  } else if (key == '\r') {
    return SelectionType::kSelected;
  } else if (key == 27) {
    return SelectionType::kNotSelected;
  }

  return SelectionType::kSelecting;
}

/**
 * \brief Handles the process of editing an existing recipient's details.
 *
 * This function allows the user to navigate through a list of recipients,
 * select one to edit, and then update details such as name, contact number,
 * site name, and group data (Maintenance, Management, Statistics). It performs
 * input validation and ensures data consistency during the editing process.
 *
 * \param key The character received from serial input to navigate or edit
 * fields.
 * \return true if the process is ongoing and further user input is required,
 * false when the editing process is complete or an error occurs.
 *
 * Functionality:
 * - Navigation: The user selects a recipient by navigating the list using the
 * '<' or '>' keys.
 * - Name Editing: The user edits the recipient's name, ensuring it’s valid and
 * unique.
 * - Contact Editing: The user updates the contact number, ensuring it’s valid
 * and unique.
 * - Site Editing: The user modifies the site name.
 * - Group Data Editing: The user toggles settings for Maintenance, Management,
 * and Statistics.
 * - Final Update: Upon completing the editing, the recipient’s data is saved
 * and updated.
 *
 * State Transitions:
 * - `INIT` -> `NAME_SELECT` -> `NAME_INPUT` -> `CONTACT_INPUT` -> `SITE_INPUT`
 * -> `GROUP_DATA_MAINTEANCE_INPUT` -> `GROUP_DATA_MANAGEMENT_INPUT` ->
 * `GROUP_DATA_STATISTICS_INPUT`
 * - Upon completion, the recipient’s information is updated, and the function
 * exits.
 *
 * \note This function ensures the recipient's data is validated and only
 * updated if all fields are correctly filled.
 */
bool ConfigManager::editRecipient(char key) {
  menuState = MenuState::kEditRecipient;

  if (infoManager.getPersonalInfoCount() == 0) {
    Serial.println("No recipients to edit.");
    return false;
  }

  switch (recipientMenuState) {
    case RecipientState::kInit:
      Serial.println(
          "Select the name to edit from the list below. Use < and > keys to "
          "navigate.\r\n");
      Serial.print("Full name: ");
      if (!infoManager.getNameAtPosition(selectedBuffer, selectedNameBuffer)) {
        return false;
      }
      Serial.print(selectedNameBuffer);
      recipientMenuState = RecipientState::kNameSelect;
      return true;

    case RecipientState::kNameSelect: {
      SelectionType selectionType = nameSelection(key, selectedNameBuffer);

      if (selectionType == SelectionType::kSelecting) {
        return true;
      } else if (selectionType == SelectionType::kSelected) {
        Serial.println(
            "\r\n\nPress any key to edit the selected field or press Enter or "
            "Esc to skip the field.\r\n");

        RecipientData *info = infoManager.searchInfo(selectedNameBuffer);
        if (info == nullptr) {
          Serial.println("Recipient details are not found.");
          return false;
        }

        selectedRecipient.contactNumber = info->contactNumber;
        selectedRecipient.groupData = info->groupData;
        selectedRecipient.name = info->name;
        selectedRecipient.siteName = info->siteName;

        tempContact = info->contactNumber;
        tempName = info->name;
        tempSite = info->siteName;
        tempMaintanence = (info->groupData & 0x01) ? 1 : 0;
        tempManagement = (info->groupData & 0x02) ? 1 : 0;
        tempStatistics = (info->groupData & 0x04) ? 1 : 0;

        Serial.print("Full name: ");
        selectionToEdit = tempName;
        Serial.print(selectionToEdit);

        editMode = EditState::kEditInit;
        recipientMenuState = RecipientState::kNameInput;
        return true;
      }
    }

    case RecipientState::kNameInput: {
      if (!editRecipientField(selectionToEdit, InputType::kAlphaNumericInput,
                              key)) {
        if (selectionToEdit.isEmpty()) {
          return false;
        }

        if (Validator::isValidName(selectionToEdit) == false) {
          Serial.println("Invalid name!");
          return false;
        }

        if (infoManager.isDuplicateEntryExists(tempName, selectionToEdit)) {
          Serial.println("Name already exists.");
          return false;
        }

        tempName = selectionToEdit;

        Serial.print("Contact number: ");
        selectionToEdit = tempContact;
        Serial.print(selectionToEdit);

        editMode = EditState::kEditInit;
        recipientMenuState = RecipientState::kContactInput;
      }

      return true;
    }
    case RecipientState::kContactInput: {
      if (!editRecipientField(selectionToEdit, InputType::kPhoneNumInput,
                              key)) {
        if (selectionToEdit.isEmpty()) {
          return false;
        }

        if (Validator::isValidPhoneNumber(selectionToEdit) == false) {
          Serial.println("Invalid contact number!");
          return false;
        }

        if (infoManager.isNumberExists(tempContact, selectionToEdit)) {
          Serial.println("Contact number already exists.");
          return false;
        }

        tempContact = selectionToEdit;

        Serial.print("Site name: ");
        selectionToEdit = tempSite;
        Serial.print(selectionToEdit);

        editMode = EditState::kEditInit;
        recipientMenuState = RecipientState::kSiteInput;
      }

      return true;
    }
    case RecipientState::kSiteInput: {
      if (!editRecipientField(selectionToEdit, InputType::kAlphaNumericInput,
                              key)) {
        if (selectionToEdit.isEmpty()) {
          return false;
        }

        tempSite = selectionToEdit;

        Serial.print("Maintenance: ");

        editMode = EditState::kEditInit;
        recipientMenuState = RecipientState::kGroupDataMaintenanceInput;

        choiceBuffer = tempMaintanence;
        getToggleInput(0, &tempMaintanence);
      }

      return true;
    }
    case RecipientState::kGroupDataMaintenanceInput: {
      if (getToggleInput(key, &tempMaintanence)) {
        Serial.print("Management: ");

        editMode = EditState::kEditInit;
        recipientMenuState = RecipientState::kGroupDataManagementInput;

        choiceBuffer = tempManagement;
        getToggleInput(0, &tempManagement);
      }

      return true;
    }
    case RecipientState::kGroupDataManagementInput: {
      if (getToggleInput(key, &tempManagement)) {
        Serial.print("Statistics: ");

        editMode = EditState::kEditInit;
        recipientMenuState = RecipientState::kGroupDataStatisticsInput;

        choiceBuffer = tempStatistics;
        getToggleInput(0, &tempStatistics);
      }

      return true;
    }
    case RecipientState::kGroupDataStatisticsInput: {
      if (getToggleInput(key, &tempStatistics)) {
        char groupData = (tempMaintanence ? 0x01 : 0x00) |
                         (tempManagement ? 0x02 : 0x00) |
                         (tempStatistics ? 0x04 : 0x00);
        infoManager.editInfo(selectedRecipient.name, tempName, tempContact,
                             tempSite, groupData);
        Serial.println("Recipient edited successfully.");
        return false;
      }

      return true;
    }
  }

  return false;
}

/**
 * \brief Manages the editing of a recipient's field (name, contact, etc.).
 *
 * This function allows the user to edit a specific recipient field by handling
 * the input character, processing it, and managing the edit state. The user can
 * cancel the edit process by pressing Enter or Escape, or they can modify the
 * field in edit mode.
 *
 * \param receipentData The recipient data to be edited. This will be updated
 * with new input.
 * \param type The expected input type (ALPHA_NUMERIC_INPUT, NUMERIC_INPUT,
 * etc.).
 * \param inKey The character received from serial input.
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
 * \note The function handles input validation and clears the recipient's data
 * when transitioning into editing mode.
 */
bool ConfigManager::editRecipientField(String &receipentData, InputType type,
                                       char inKey) {
  if ((editMode != EditState::kEditing) && (inKey == '\r' || inKey == 27)) {
    Serial.println();
    editMode = EditState::kEditInit;
    return false;
  }

  if (editMode == EditState::kEditInit && isValidChar(type, inKey)) {
    clearRecepientNameInEditMode(receipentData);

    editMode = EditState::kEditing;

    receipentData = "";
    inputBuffer = inKey;

    Serial.print(inKey);
    return true;
  }

  if (editMode == EditState::kEditing) {
    if (processInputBuffer(type, inKey, receipentData)) {
      editMode = EditState::kEditInit;
      return false;
    }

    return true;
  }

  return false;
}

/**
 * \brief Displays the detailed information of a recipient on the specified
 * serial display.
 *
 * This function prints the recipient's full name, contact number, site name,
 * and group data (Maintenance, Management, and Statistics) to the provided
 * serial display.
 *
 * \param info The `RecipientData` structure containing the recipient's
 * information to be displayed.
 *
 * Functionality:
 * - Displays the recipient's **Full Name**, **Contact Number**, and **Site
 * Name**.
 * - Displays the **Maintenance**, **Management**, and **Statistics** group data
 * as either "YES" or "NO" based on bit flags.
 * - Separates the data with a dashed line for clarity.
 *
 * \note This function is used to show the detailed recipient information on an
 * external display via Serial.
 */
void ConfigManager::showPersonalInfo(const RecipientData &info) {
  Serial.println("Full name: " + info.name);
  Serial.println("Contact: " + info.contactNumber);
  Serial.println("Site: " + info.siteName);

  Serial.print("Maintenance: ");
  Serial.println((info.groupData & 0x01) ? "YES" : "NO");

  Serial.print("Management: ");
  Serial.println((info.groupData & 0x02) ? "YES" : "NO");

  Serial.print("Statistics: ");
  Serial.println((info.groupData & 0x04) ? "YES" : "NO");

  Serial.println("-------------------");
}

/**
 * \brief Deletes a recipient from the list based on user selection.
 *
 * This function allows the user to select a recipient to delete from the list.
 * The user can navigate through the list using the '<' and '>' keys, select a
 * recipient, and then confirm deletion. If no recipients are available, a
 * message is shown.
 *
 * \param key The character received from serial input to navigate or confirm
 * the deletion.
 * \return true if the user is still in the selection process and further input
 * is required, false when the deletion process is completed or canceled.
 *
 * Functionality:
 * - Navigation: The user selects a recipient to delete using the '<' or '>'
 * keys.
 * - Deletion: Upon selection, the recipient’s information is deleted from the
 * list if found.
 * - Confirmation: Success or failure messages are shown after attempting to
 * delete the recipient.
 *
 * \note This function interacts with the `infoManager` to delete the selected
 * recipient’s data.
 */
bool ConfigManager::deleteRecipient(char key) {
  menuState = MenuState::kDeleteRecipient;

  if (infoManager.getPersonalInfoCount() == 0) {
    Serial.println("No recipients to delete.");
    return false;
  }

  if (recipientMenuState == RecipientState::kInit) {
    Serial.println(
        "Select the name to delete from the list below. Use < and > keys to "
        "navigate.\r\n");
    Serial.print("Full name: ");
    if (!infoManager.getNameAtPosition(selectedBuffer, selectedNameBuffer)) {
      return false;
    }
    Serial.print(selectedNameBuffer);
    recipientMenuState = RecipientState::kNameSelect;
    return true;
  }

  if (recipientMenuState == RecipientState::kNameSelect) {
    SelectionType selectionType = nameSelection(key, selectedNameBuffer);

    if (selectionType == SelectionType::kSelecting) {
      return true;
    } else if (selectionType == SelectionType::kSelected) {
      if (selectedNameBuffer.isEmpty()) {
        return false;
      }

      Serial.println();

      if (infoManager.deleteInfo(selectedNameBuffer) == 0) {
        Serial.println("Failed to delete recipient.");
      } else {
        Serial.println("Recipient deleted successfully.");
      }
    }
  }

  return false;
}

/**
 * \brief Searches for a recipient based on the provided name.
 *
 * This function allows the user to search for a recipient by entering their
 * name. If the recipient is found, their details are displayed. If no
 * recipients are available, or if no match is found, an appropriate message is
 * shown.
 *
 * \param key The character received from serial input, used to build the search
 * query.
 * \return true if the user is still in the process of entering the name,
 *         false when the search is complete or canceled.
 *
 * Functionality:
 * - In the initial state (`NAME_INPUT`), the user inputs the recipient's name.
 * - If a match is found, the recipient's details are displayed using
 * `showPersonalInfo`.
 * - If no match is found, an error message is displayed.
 * - If no recipients are available in the list, a message indicating this is
 * shown.
 *
 * \note This function interacts with the `infoManager` to perform the search
 * and retrieve recipient information.
 */
bool ConfigManager::searchRecipient(char key) {
  menuState = MenuState::kSearchRecipient;

  if (infoManager.getPersonalInfoCount() == 0) {
    Serial.println("No recipients to search.");
    return false;
  }

  if (recipientMenuState == RecipientState::kInit) {
    Serial.print("Enter the name to search: ");
    recipientMenuState = RecipientState::kNameInput;
    return true;
  }

  if (recipientMenuState == RecipientState::kNameInput) {
    if (processInputBuffer(InputType::kAlphaNumericInput, key, tempName)) {
      if (tempName.isEmpty()) {
        return false;
      }

      RecipientData *info = infoManager.searchInfo(tempName);
      if (info == nullptr) {
        Serial.println("Recipient details are not found.");
        return false;
      }

      showPersonalInfo(*info);
      return false;
    }

    return true;
  }

  return false;
}

/**
 * \brief Displays a list of all recipients.
 *
 * This function displays all the recipients in the system. If no recipients are
 * available, it will show a message indicating that there are no recipients to
 * display. If recipients exist, their details are printed using the
 * `showPersonalInfo` function.
 *
 * \note This function relies on `infoManager` to retrieve and print the
 * recipients' details.
 */
void ConfigManager::showAllRecipient() {
  if (infoManager.getPersonalInfoCount() == 0) {
    Serial.println("No recipients to show.");
    return;
  }

  infoManager.printAll(showPersonalInfo);
}

/**
 * \brief Prints the main configuration menu to the serial display.
 *
 * This function displays a menu with options for managing recipient data. The
 * available options include viewing the recipient list, adding, editing,
 * deleting, searching for recipients, and exiting the configuration menu.
 *
 * \note The `menuState` is updated to `MAIN_MENU` upon entering this function,
 * and the options are displayed to guide the user in making a selection.
 */
void ConfigManager::printMenu() {
  menuState = MenuState::kMainMenu;

  Serial.println("\r\nDevice Configuration Menu\r\n");
  Serial.println("[1] Show Recipient List");
  Serial.println("[2] Add Recipient");
  Serial.println("[3] Edit Recipient Details");
  Serial.println("[4] Remove Recipient");
  Serial.println("[5] Search Recipient");
#ifdef RTC_MANAGER
  Serial.println("[6] Set System Date/Time");
  Serial.println("[7] Show System Date/Time");
#endif
  Serial.println("[8] Show Log Entries");
  Serial.println("[X] Exit Configuration Menu");
  Serial.print("\r\nSelection: ");
}

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
  menuState = MenuState::kDateTimeSet;

  switch (dateInputStage) {
    case SetDateTimeStage::SDT_INIT:
      Serial.print("Year: ");
      dateInputStage = SetDateTimeStage::SDT_YEAR;
      break;
    case SetDateTimeStage::SDT_YEAR:
      if (processInputBuffer(InputType::kNumberInput, key, tempNumber)) {
        if (tempNumber.isEmpty()) return false;

        if (!TimeManager::isValidYear(tempNumber)) {
          Serial.println("Invalid year!");
          return false;
        }

        inputYear = tempNumber.toInt();

        dateInputStage = SetDateTimeStage::SDT_MONTH;
        tempNumber = "";
        Serial.print("Month: ");

        return true;
      }
      break;

    case SetDateTimeStage::SDT_MONTH:
      if (processInputBuffer(InputType::kNumberInput, key, tempNumber)) {
        if (tempNumber.isEmpty()) return false;

        if (!TimeManager::isValidMonth(tempNumber)) {
          Serial.println("Invalid month!");
          return false;
        }

        inputMonth = tempNumber.toInt();

        dateInputStage = SetDateTimeStage::SDT_DAY;
        tempNumber = "";
        Serial.print("Day: ");

        return true;
      }
      break;

    case SetDateTimeStage::SDT_DAY:
      if (processInputBuffer(InputType::kNumberInput, key, tempNumber)) {
        if (tempNumber.isEmpty()) return false;

        if (!TimeManager::isValidDay(tempNumber, inputYear, inputMonth)) {
          Serial.println("Invalid day!");
          return false;
        }

        inputDay = tempNumber.toInt();

        dateInputStage = SetDateTimeStage::SDT_HOUR;
        tempNumber = "";
        Serial.print("Hour: ");

        return true;
      }
      break;
    case SetDateTimeStage::SDT_HOUR:
      if (processInputBuffer(InputType::kNumberInput, key, tempNumber)) {
        if (tempNumber.isEmpty()) return false;

        if (!TimeManager::isValidHour(tempNumber)) {
          Serial.println("Invalid hour!");
          return false;
        }

        inputHour = tempNumber.toInt();

        dateInputStage = SetDateTimeStage::SDT_MINUTE;
        tempNumber = "";
        Serial.print("Minute: ");

        return true;
      }
      break;
    case SetDateTimeStage::SDT_MINUTE:
      if (processInputBuffer(InputType::kNumberInput, key, tempNumber)) {
        if (tempNumber.isEmpty()) return false;

        if (!TimeManager::isValidMinute(tempNumber)) {
          Serial.println("Invalid minute!");
          return false;
        }

        inputMinute = tempNumber.toInt();

        dateInputStage = SetDateTimeStage::SDT_SECOND;
        tempNumber = "";
        Serial.print("Second: ");

        return true;
      }
      break;
    case SetDateTimeStage::SDT_SECOND:
      if (processInputBuffer(InputType::kNumberInput, key, tempNumber)) {
        if (tempNumber.isEmpty()) return false;

        // Check if the second is valid (using the same function as minute).
        if (!TimeManager::isValidMinute(tempNumber)) {
          Serial.println("Invalid second!");
          return false;
        }

        inputSecond = tempNumber.toInt();

        TimeManager::setSystemTime(inputYear, inputMonth, inputDay, inputHour,
                                   inputMinute, inputSecond);

        dateInputStage = SetDateTimeStage::SDT_INIT;
        tempNumber = "";
        Serial.println("Date/Time set successfully.");
        return false;
      }
      break;
  }

  return true;
}

/**
 * \brief Resets the internal states and buffers to their initial values.
 *
 * This function resets all relevant variables and states used in the recipient
 * management process to their default or initial values. It ensures that any
 * data entered or selected during previous operations is cleared, effectively
 * resetting the configuration state.
 *
 * \note This function is useful for starting fresh or after completing certain
 * actions like adding, editing, or deleting recipients, ensuring no residual
 * data interferes with future operations.
 */
void ConfigManager::resetSubState() {
  recipientMenuState = RecipientState::kInit;
  inputBuffer = "";
  choiceBuffer = 0;
  selectedNameBuffer = "";
  selectedBuffer = 0;
  selectionToEdit = "";
  editMode = EditState::kEditInit;

  tempName = "";
  tempContact = "";
  tempSite = "";
  tempMaintanence = 0;
  tempManagement = 0;
  tempStatistics = 0;

  selectedRecipient.contactNumber = "";
  selectedRecipient.groupData = 0;
  selectedRecipient.name = "";
  selectedRecipient.siteName = "";

  dateInputStage = SetDateTimeStage::SDT_INIT;
}

/**
 * \brief Handles user input and processes menu options.
 *
 * This function processes a single user input, performs the corresponding
 * action based on the menu selection, and updates the menu state accordingly.
 * It supports operations like showing all recipients, adding, editing,
 * deleting, or searching recipients, and exiting the configuration menu.
 *
 * \param input The character representing the user's menu selection.
 *
 * \note After processing a valid input, the menu state is updated, and the main
 * menu is reprinted unless the operation is still in progress (e.g., adding or
 * editing a recipient).
 */
void ConfigManager::handleInput(char input) {
  bool isStillInState = false;
  resetSubState();

  switch (input) {
    case '1':
      showAllRecipient();
      break;
    case '2':
      isStillInState = addRecipient(input);
      break;
    case '3':
      isStillInState = editRecipient(input);
      break;
    case '4':
      isStillInState = deleteRecipient(input);
      break;
    case '5':
      isStillInState = searchRecipient(input);
      break;
#ifdef RTC_MANAGER
    case '6':
      isStillInState = setSystemDateTime(input);
      break;
    case '7':
      TimeManager::showSystemDateTime();
      break;
#endif
    case '8':
      LoggingManager::showAllLogs();
      break;
    case 'x':
    case 'X':
      Serial.println("Exiting configuration menu.");
      menuState = MenuState::kIdle;
      return;
    default:
      Serial.println("Invalid option.");
      printMenu();
      break;
  }

  if (!isStillInState) {
    printMenu();
  }
}

}  // namespace inamata