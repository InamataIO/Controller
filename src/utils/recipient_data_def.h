#pragma once

#include <Arduino.h>

// Maximum number of personal information entries.
#define MAX_PERSONAL_INFO_COUNT 10

namespace inamata {

// Structure to hold personal information.
struct RecipientData {
  String name;           // Recipient's name.
  String contactNumber;  // Contact number.
  String siteName;       // Site name.

  // Group data for classification in each bit field:
  // [0] - Maintenance
  // [1] - Management
  // [2] - Statistics
  // [3..7] - Reserved for future use.
  char groupData;  // Group data as a bit field.
};

}  // namespace inamata