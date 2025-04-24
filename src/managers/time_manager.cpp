#include "time_manager.h"

#include <Arduino.h>
#include <RTClib.h>

namespace inamata {

RTC_DS3231 rtc;

/**
 * \brief Initializes the RTC module.
 *
 * This function initializes the RTC module and checks if it is present. If the
 * RTC module is not found, an error message is printed to the serial monitor.
 */
void TimeManager::initRTC() {
  if (!rtc.begin()) {
    Serial.println("RTC not found");
    return;
  }
}

/**
 * \brief Gets the current system time.
 *
 * This function retrieves the current system time from the RTC module.
 *
 * \return The current system time as a DateTime object.
 */
DateTime TimeManager::systemTime() { return rtc.now(); }

/**
 * \brief Formats the current system time as a string.
 *
 * This function formats the current system time into a string in the format
 * "YYYY-MM-DDTHH:MM:SS". Useful for logging and displaying the current time in
 * ISO 8601 format.
 *
 * \return The formatted time as a string (in ISO 8601 format).
 */
String TimeManager::getFormattedTime() {
  char buffer[20];
  DateTime st = systemTime();

  snprintf(buffer, sizeof(buffer), "%04d-%02d-%02dT%02d:%02d:%02d", st.year(),
           st.month(), st.day(), st.hour(), st.minute(), st.second());

  return String(buffer);
}

/**
 * \brief Sets the system time.
 *
 * This function sets the system time using the provided year, month, day, hour,
 * minute, and second values.
 *
 * \param year The year to set.
 * \param month The month to set.
 * \param day The day to set.
 * \param hour The hour to set.
 * \param minute The minute to set.
 * \param second The second to set.
 */
void TimeManager::setSystemTime(int year, int month, int day, int hour,
                                int minute, int second) {
  rtc.adjust(DateTime(year, month, day, hour, minute, second));
}

/**
 * \brief Gets the current date in "YYYY-MM-DD" format.
 *
 * This function retrieves the current date from the RTC module and formats it
 * as a string.
 *
 * \return The current date as a string (in "YYYY-MM-DD" format).
 */
String TimeManager::getCurrentDate() {
  char buffer[11];
  DateTime st = systemTime();

  snprintf(buffer, sizeof(buffer), "%04d%02d%02d", st.year(), st.month(),
           st.day());

  return String(buffer);
}

/**
 * \brief Gets a date that is a specified number of days in the past.
 *
 * This function calculates a date that is a specified number of days in the
 * past from the current system time.
 *
 * \param days The number of days to go back in time.
 * \return The calculated past date as a DateTime object.
 */
DateTime TimeManager::getPastDate(int days) {
  DateTime current = systemTime();
  return current - TimeSpan(days * SECONDS_PER_DAY);
}

/**
 * \brief Displays the current system date and time.
 *
 * This function prints the current system date and time to the serial monitor
 * in a human-readable format.
 */
void TimeManager::showSystemDateTime() {
  DateTime now = systemTime();
  char buffer[34];

  snprintf(buffer, sizeof(buffer),
           "Date & Time: %04d-%02d-%02d %02d:%02d:%02d\n", now.year(),
           now.month(), now.day(), now.hour(), now.minute(), now.second());

  Serial.println(buffer);
}

/**
 * \brief Validates the year input.
 *
 * This function checks if the provided year string is valid (4 digits and in
 * the range of 2000 to 3000).
 *
 * \param year The year string to validate.
 * \return true if the year is valid, false otherwise.
 */
bool TimeManager::isValidYear(String year) {
  if (year.length() != 4) {
    return false;
  }

  for (char c : year) {
    if (!isdigit(c)) {
      return false;
    }
  }

  int yearValue = year.toInt();
  return (yearValue >= 2000 && yearValue <= 3000);
}

/**
 * \brief Validates the month input.
 *
 * This function checks if the provided month string is valid (1 or 2 digits and
 * in the range of 1 to 12).
 *
 * \param month The month string to validate.
 * \return true if the month is valid, false otherwise.
 */
bool TimeManager::isValidMonth(String month) {
  if (month.length() > 2 || month.length() < 1) {
    return false;
  }

  for (char c : month) {
    if (!isdigit(c)) {
      return false;
    }
  }

  int monthValue = month.toInt();
  return (monthValue >= 1 && monthValue <= 12);
}

/**
 * \brief Validates the day input.
 *
 * This function checks if the provided day string is valid (1 or 2 digits and
 * in the range of 1 to 31, considering the month and leap years).
 *
 * \param day The day string to validate.
 * \param inYear The year to consider for leap year calculation.
 * \param inMonth The month to consider for day validation.
 * \return true if the day is valid, false otherwise.
 */
bool TimeManager::isValidDay(String day, int inYear, int inMonth) {
  if (day.length() > 2 || day.length() < 1) {
    return false;
  }

  for (char c : day) {
    if (!isdigit(c)) {
      return false;
    }
  }

  int dayValue = day.toInt();

  if (inMonth < 1 || inMonth > 12) {
    return false;
  }

  if (inMonth == 2) {
    // Check for leap year
    bool isLeapYear =
        (inYear % 4 == 0 && inYear % 100 != 0) || (inYear % 400 == 0);
    return (dayValue >= 1 && dayValue <= (isLeapYear ? 29 : 28));
  } else if (inMonth == 4 || inMonth == 6 || inMonth == 9 || inMonth == 11) {
    return (dayValue >= 1 && dayValue <= 30);
  } else {
    return (dayValue >= 1 && dayValue <= 31);
  }
  return false;
}

/*
 * \brief Validates the hour input.
 *
 * This function checks if the provided hour string is valid (1 or 2 digits and
 * in the range of 0 to 23).
 *
 * \param hour The hour string to validate.
 * \return true if the hour is valid, false otherwise.
 */
bool TimeManager::isValidHour(String hour) {
  if (hour.length() > 2 || hour.length() < 1) {
    return false;
  }

  for (char c : hour) {
    if (!isdigit(c)) {
      return false;
    }
  }

  int hourValue = hour.toInt();
  return (hourValue >= 0 && hourValue <= 23);
}

/*
 * \brief Validates the minute seconds input.
 *
 * This function checks if the provided minute (or seconds) string is valid (1
 * or 2 digits and in the range of 0 to 59).
 *
 * \param minute The minute (or seconds) string to validate.
 * \return true if the minute (or seconds) is valid, false otherwise.
 */
bool TimeManager::isValidMinute(String minute) {
  if (minute.length() > 2 || minute.length() < 1) {
    return false;
  }

  for (char c : minute) {
    if (!isdigit(c)) {
      return false;
    }
  }

  int minuteValue = minute.toInt();
  return (minuteValue >= 0 && minuteValue <= 59);
}

}  // namespace inamata.