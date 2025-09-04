#ifdef RTC_MANAGER

#include "time_manager.h"

#include <Arduino.h>
#include <RTClib.h>

#include "managers/logging.h"

namespace inamata {

RTC_DS3231 rtc;

/**
 * \brief Initializes the RTC module.
 *
 * This function initializes the RTC module and checks if it is present. If the
 * RTC module is not found, an error message is printed to the serial monitor.
 *
 * Also checks if the RTC lost power since the last power off. Useful to check
 * if battery backing RTC is dead
 */
void TimeManager::initRTC() {
  if (!rtc.begin()) {
    Serial.println("RTC not found");
    return;
  }
  lost_power_on_boot_ = lostPower();
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
String TimeManager::getFormattedTime(const DateTime &date_time) {
  // Oversize to have buffer for max integer sizes (uint8_t -> 3 chars)
  char buffer[28];

  snprintf(buffer, sizeof(buffer), "%04d-%02d-%02dT%02d:%02d:%02d",
           date_time.year(), date_time.month(), date_time.day(),
           date_time.hour(), date_time.minute(), date_time.second());

  return buffer;
}

String TimeManager::getFormattedTime() {
  return getFormattedTime(systemTime());
}

/**
 * \brief Sets the system time.
 *
 * \param date_time The DateTime object with a specified date and time
 */
void TimeManager::setSystemTime(const DateTime &date_time) {
  rtc.adjust(date_time);
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

bool TimeManager::lostPower() { return rtc.lostPower(); }

/**
 * \brief Gets the current date in "YYYYMMDD" format.
 *
 * This function retrieves the current date from the RTC module and formats it
 * as a string.
 *
 * \param date_time Datetime to be printed
 * \return The current date as a string (in "YYYYMMDD" format).
 */
String TimeManager::getCurrentDate(const DateTime &date_time) {
  // Oversize to have buffer for max integer sizes (uint8_t -> 3 chars)
  char buffer[11];

  snprintf(buffer, sizeof(buffer), "%04d%02d%02d", date_time.year(),
           date_time.month(), date_time.day());

  return buffer;
}

/**
 * \brief Gets the current date in "YYYY-MM-DD" format.
 *
 * \see getCurrentDate(const DateTime &)
 * \return The current date as a string (in "YYYY-MM-DD" format).
 */
String TimeManager::getCurrentDate() { return getCurrentDate(systemTime()); }

/**
 * \brief Parses data during provisioning and sets the RTC
 *
 * Checked with the following 'now' values to work:
 *   2025-02-01T12:01:06
 *   2025-01-01T12:01:06
 *   2025-12-01T12:01:06
 *   2025-02-31T12:01:06
 *   2025-02-01T23:01:06
 *   2025-02-01T12:59:06
 *   2025-02-01T12:59:59
 * To fail on:
 *   2024-02-01T12:01:06
 *   2025--02-01T12:01:0
 *   2025-00-01T12:01:06
 *   2025-13-01T12:01:06
 *   2025-02-00T12:01:06
 *   2025-02-32T12:01:06
 *   2025-02-01T-1:01:06
 *   2025-02-01T24:01:06
 *   2025-02-01T12:-1:06
 *   2025-02-01T12:60:06
 *   2025-02-01T12:01:-1
 *   2025-02-01T12:01:60
 *
 * Checked with the following 'utc_offset' values to work:
 *   -1:30
 *   -10:00
 *   -10:59
 *   1:30
 *   10:00
 *   0:30
 *   0:00
 *   +1:30
 *   +12:00
 *   +0:30
 *   +00:30
 *
 * \return True on success
 */
bool TimeManager::handleImprovUserData(const JsonObjectConst &data) {
  JsonObjectConst time = data["time"].as<JsonObjectConst>();
  if (time.isNull() || time.size() == 0) {
    TRACELN("No time");
    return true;
  }

  JsonVariantConst now = time["now"];
  if (!now.is<const char *>()) {
    TRACELN("No now");
    return false;
  }
  DateTime date_time(now.as<const char *>());
  if (!date_time.isValid()) {
    TRACELN("Parsing failed");
    return false;
  }

  JsonVariantConst utc_offset = time["utc_offset"];
  if (utc_offset.is<const char *>()) {
    int offset_hour, offset_minute;
    char sign = '+';
    int result = sscanf(utc_offset.as<const char *>(), "%c%d:%d", &sign,
                        &offset_hour, &offset_minute);
    if (result != 3 || !(sign == '-' || sign == '+')) {
      result = sscanf(utc_offset.as<const char *>(), "%d:%d", &offset_hour,
                      &offset_minute);
      sign = '+';
    }
    TimeSpan time_offset;
    if (result == 2 || result == 3) {
      time_offset = TimeSpan(0, offset_hour, offset_minute, 0);
      if (sign == '+') {
        date_time = date_time + time_offset;
      } else if (sign == '-') {
        date_time = date_time - time_offset;
      } else {
        TRACEF("Invalid sign %c\r\n", sign);
      }
    }
  }
  rtc.adjust(date_time);
  return true;
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

  int year_value = year.toInt();
  return (year_value >= 2000 && year_value <= 3000);
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

  int month_value = month.toInt();
  return (month_value >= 1 && month_value <= 12);
}

/**
 * \brief Validates the day input.
 *
 * This function checks if the provided day string is valid (1 or 2 digits and
 * in the range of 1 to 31, considering the month and leap years).
 *
 * \param day The day string to validate.
 * \param in_year The year to consider for leap year calculation.
 * \param in_month The month to consider for day validation.
 * \return true if the day is valid, false otherwise.
 */
bool TimeManager::isValidDay(String day, int in_year, int in_month) {
  if (day.length() > 2 || day.length() < 1) {
    return false;
  }

  for (char c : day) {
    if (!isdigit(c)) {
      return false;
    }
  }

  int day_value = day.toInt();

  if (in_month < 1 || in_month > 12) {
    return false;
  }

  if (in_month == 2) {
    // Check for leap year
    bool isLeapYear =
        (in_year % 4 == 0 && in_year % 100 != 0) || (in_year % 400 == 0);
    return (day_value >= 1 && day_value <= (isLeapYear ? 29 : 28));
  } else if (in_month == 4 || in_month == 6 || in_month == 9 ||
             in_month == 11) {
    return (day_value >= 1 && day_value <= 30);
  } else {
    return (day_value >= 1 && day_value <= 31);
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

  int hour_value = hour.toInt();
  return (hour_value >= 0 && hour_value <= 23);
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

  int minute_value = minute.toInt();
  return (minute_value >= 0 && minute_value <= 59);
}

bool TimeManager::isLostPowerOnBoot() { return lost_power_on_boot_; }

bool TimeManager::lost_power_on_boot_ = false;

}  // namespace inamata.

#endif