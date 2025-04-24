#pragma once

#include <Arduino.h>

#include <memory>

#include "utils/person_info.h"

namespace inamata {

class ConfigManager {
 private:
  enum class InputType {
    kAlphaNumericInput,  // Allows alphanumeric characters.
    kPhoneNumInput,      // Allows only characters which uses in phone numbers.
    kAnyInput,           // Allows any printable character.
    kNumberInput         // Allows only numbers.
  };

  enum class MenuState {
    kIdle,             // In idle mode (Main state machine is not yet started).
    kMainMenu,         // Main selection state.
    kAddRecipient,     // Add recipient state.
    kEditRecipient,    // Edit recipient state.
    kDeleteRecipient,  // Delete recipient state.
    kSearchRecipient,  // Search recipient state.
    kDateTimeSet,      // Set date and time state.
    kExit              // Exit from configuration mode.
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

  enum class RecipientState {
    kInit,          // Initializing state.
    kNameSelect,    // Name selection state (in edit or delete flows).
    kNameInput,     // Name input state.
    kContactInput,  // Input contact number state.
    kSiteInput,     // Location input state.
    kGroupDataMaintenanceInput,  // Setting up maintenance flag.
    kGroupDataManagementInput,   // Setting up management flag.
    kGroupDataStatisticsInput    // Setting up statistics flag.
  };

  enum class SetDateTimeStage {
    SDT_INIT,     // Initial state.
    SDT_YEAR,     // Year input state.
    SDT_MONTH,    // Month input state.
    SDT_DAY,      // Day input state.
    SDT_HOUR,     // Hour input state.
    SDT_MINUTE,   // Minute input state.
    SDT_SECOND    // Second input state.
  };

  // configuration manager's serial operations.
  String inputBuffer;         // Buffer to store raw user input.
  char choiceBuffer;          // Buffer to store last user choice.
  int selectedBuffer;         // Buffer to store selected recipient index (in
                              // edit and delete modes).
  String selectedNameBuffer;  // Buffer to store selected recipient name.
  RecipientData selectedRecipient;  // Pointer to the selected recipient data.
  String selectionToEdit;  // Buffer to store the selected recipient to edit.
  EditState editMode;      // Edit mode state.
  PersonalInfoManager
      infoManager;  // Manages personal information collection of recipients.

  String tempName;     // Name of the recipient.
  String tempContact;  // Contact number of the recipient.
  String tempSite;     // Site name of the recipient.
  String tempNumber;  // Temporary buffer to store the numbers input by the user
                      // (mainly used in date/time input routines).
  char tempMaintanence;  // Maintenance flag (extracted from group data).
  char tempManagement;   // Management flag (extracted from group data).
  char tempStatistics;   // Statistics flag (extracted from group data).

  MenuState menuState;  // Current state of the menu.
  RecipientState
      recipientMenuState;  // Current state of the add recipient menu.

  SetDateTimeStage
      dateInputStage;  // Current stage of the date/time input process.
  int inputYear, inputMonth, inputDay, inputHour, inputMinute,
      inputSecond;  // Components of date/time input flow.

  void printMenu();
  void resetSubState();
  void handleInput(char input);

  void showAllRecipient();
  bool addRecipient(char key);
  void clearRecepientNameInEditMode(String &name);
  SelectionType nameSelection(char key, String &lastName);
  bool editRecipient(char key);

  bool editRecipientField(String &receipentData, InputType type, char inKey);

  bool processInputBuffer(InputType type, char inKey, String &output);
  bool getToggleInput(char inKey, char *returnChoice);
  bool isValidChar(InputType type, char c);

  bool deleteRecipient(char key);
  bool searchRecipient(char key);

  bool setSystemDateTime(char key);

 public:
  void initConfigMenuManager();
  void loop();

  // Static function to display recipient details.
  static void showPersonalInfo(const RecipientData &info);
};

}  // namespace inamata