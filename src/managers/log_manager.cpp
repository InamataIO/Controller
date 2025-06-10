#ifdef RTC_MANAGER

#include "log_manager.h"

#include <Arduino.h>
#include <LittleFS.h>

#include <algorithm>
#include <vector>

#include "managers/logging.h"
#include "managers/storage.h"
#include "time_manager.h"

namespace inamata {

/**
 * @brief LoggingManager constructor
 *
 * Initializes the SD card and creates the log directory structure.
 *
 * It creates the root log directory (/logs) and the device-specific log
 * directory (/logs/fdl). It also finds the next available log file number
 * and deletes log files older than RETENTION_DAYS.
 *
 * @note The log file number is incremented until an available file name is
 * found. The maximum file number is 255.
 * @note The log directory name is based on the current date in the format
 * YYYYMMDD.
 *
 * @note The log file name format is /logs/fdl/YYYYMMDD/N.log, where N is the
 * next available file number.
 */
LoggingManager::LoggingManager() : file_number_(0), log_count_(0) {
  char buffer[96];

  if (!LittleFS.begin(true)) {
    Serial.println("External flash storage mount Failed");
  }

  // Root level log directory (/logs).
  if (!LittleFS.exists("/logs")) {
    if (!LittleFS.mkdir("/logs")) {
      Serial.println("Failed to create root logs directory");
    }
  }

  // Device specific log directory (/logs/fdl).
  if (!LittleFS.exists(LoggingManager::root_path_)) {
    if (!LittleFS.mkdir(LoggingManager::root_path_)) {
      Serial.println("Failed to create log directory");
    }
  }

  // Find the log file name to start from the next available number.
  do {
    file_number_++;
    snprintf(buffer, sizeof(buffer), "%s/%s/%d.log", LoggingManager::root_path_,
             TimeManager::getCurrentDate().c_str(), file_number_);
  } while (LittleFS.exists(buffer));
}

/**
 * @brief Rotate the log file.
 *
 * Increments the file number and resets the log count. This is called when
 * the maximum log size is reached.
 */
void LoggingManager::rotateLogFile() {
  file_number_++;
  log_count_ = 0;
}

/**
 * @brief Delete old log files.
 *
 * Deletes log files if the total storage exceeds kMaxTotalLogBytes. The log
 * file name format is /logs/fdl/YYYYMMDD/N.log, where N is the next available
 * file number.
 *
 * @note The log directory name is based on the current date in the format
 * YYYYMMDD.
 */
void LoggingManager::deleteOldLogs() {
  File root = LittleFS.open(LoggingManager::root_path_, "r+");
  if (!root || !root.isDirectory()) {
    TRACEF("Can't open dir %s\n", LoggingManager::root_path_);
    root.close();
    return;
  }
  long oldest_date = 0;
  size_t total_log_size = 0;

  // Calculate the total log file sizes, keep track of the oldest directory
  for (File log_dir = root.openNextFile(); log_dir;
       log_dir = root.openNextFile()) {
    if (!log_dir.isDirectory()) {
      continue;
    }
    char* endptr;
    const long date_int = strtoul(log_dir.name(), &endptr, 10);
    if (*endptr != '\0') {
      TRACEF("Parsing date to int failed for %s\n", log_dir.name());
      continue;
    }
    if (oldest_date == 0) {
      oldest_date = date_int;
      TRACEF("Init oldest_date: %lu\n", oldest_date);
    } else if (oldest_date > date_int) {
      TRACEF("Update oldest_date %lu -> %lu\n", oldest_date, date_int);
      oldest_date = date_int;
    }

    // Sum the size of all logs for a given day
    for (File log_file = log_dir.openNextFile(); log_file;
         log_file = log_dir.openNextFile()) {
      if (log_file.isDirectory()) {
        TRACEF("Unexpected dir: %s\n", log_file.path());
        continue;
      }
      total_log_size += log_file.size();
    }
    TRACEF("Total log size: %lu\n", total_log_size);
  }

  if (total_log_size > kMaxTotalLogBytes) {
    String delete_path =
        String(LoggingManager::root_path_) + '/' + String(oldest_date);
    TRACEF("Deleting %s\n", delete_path.c_str());
    Storage::recursiveRm(delete_path.c_str());
  }
  root.close();
}

/**
 * @brief Add a log entry.
 *
 * Adds a log entry to the log file. The log entry format is
 * YYYY-MM-DDTHH:MM:SS - event - status.
 *
 * @param event The event to log.
 */
void LoggingManager::addLog(const String& event) {
  char buffer[128];

  snprintf(buffer, sizeof(buffer), "%s/%s", LoggingManager::root_path_,
           TimeManager::getCurrentDate().c_str());

  String logEntry = TimeManager::getFormattedTime() + "," + event;

  // Create the daily log directory if it doesn't exist (can happen if clock is
  // running closer to 24:00)
  if (!LittleFS.exists(buffer)) {
    if (!LittleFS.mkdir(buffer)) {
      Serial.println("Failed to create daily log directory:");
      Serial.println(buffer);
      return;
    }
  }

  snprintf(buffer, sizeof(buffer), "%s/%s/%d.log", LoggingManager::root_path_,
           TimeManager::getCurrentDate().c_str(), file_number_);

  fs::File file = LittleFS.open(buffer, FILE_APPEND);
  if (!file) {
    file = LittleFS.open(buffer, FILE_WRITE);
    if (!file) {
      Serial.println("Failed to open log file");
      Serial.println(buffer);
      return;
    }
  }

#ifdef ENABLE_TRACE
  Serial.print("Current Log File: ");
  Serial.println(buffer);
#endif

  file.println(logEntry.c_str());

#ifdef ENABLE_TRACE
  Serial.print("Log Entry: ");
  Serial.println(logEntry);
#endif

  file.close();

  log_count_++;
  if (log_count_ >= kMaxLogEntries) {
    rotateLogFile();
  }
}

/**
 * @brief Show the logs.
 *
 * Reads and prints the log file to the serial console. The log file name
 * format is /logs/fdl/YYYYMMDD/N.log, where N is the next available file
 * number.
 */
void LoggingManager::showCurrentLog() {
  char buffer[96];

  snprintf(buffer, sizeof(buffer), "%s/%s/%d.log", LoggingManager::root_path_,
           TimeManager::getCurrentDate().c_str(), file_number_);

  fs::File file = LittleFS.open(buffer, FILE_READ);
  if (!file) {
    Serial.println("Failed to open log file");
    return;
  }

  while (file.available()) {
    Serial.println(file.readStringUntil('\n'));
  }

  file.close();
}

/**
 * @brief Get all log paths.
 *
 * Reads all log files in the log directory and returns a vector of
 * LogPath structs containing the date, file number, and full path of each
 * log file.
 *
 * @return A vector of LogPath structs containing the date, file number,
 * and full path of each log file.
 */
std::vector<LoggingManager::LogPath> LoggingManager::getAllLogPaths() {
  std::vector<LoggingManager::LogPath> paths;

  File root = LittleFS.open(LoggingManager::root_path_);
  if (!root || !root.isDirectory()) {
    Serial.println("Failed to open /logs/fdl or not a directory.");
    return paths;
  }

  File date_dir;
  while ((date_dir = root.openNextFile())) {
    if (!date_dir.isDirectory()) {
      date_dir.close();
      continue;
    }

    File log_file;
    while ((log_file = date_dir.openNextFile())) {
      String name = log_file.name();

      if (name.endsWith(".log")) {
        int file_num = name.substring(0, name.indexOf(".log")).toInt();
        LogPath lp;
        lp.date_time = date_dir.name();
        lp.file_num = file_num;
        paths.push_back(lp);
      }

      log_file.close();
    }

    date_dir.close();
  }

  // Sort: first by date descending, then by file number descending.
  std::sort(paths.begin(), paths.end(), [](const LogPath& a, const LogPath& b) {
    if (a.date_time == b.date_time) return a.file_num > b.file_num;
    return a.date_time > b.date_time;
  });

  return paths;
}

/**
 * @brief Show all logs.
 *
 * Reads and prints all log files in the log directory to the serial
 * console.
 */
void LoggingManager::showAllLogs() {
  auto logPaths = getAllLogPaths();

  Serial.println("Date/Time,Action,State");

  for (const auto& log : logPaths) {
    File f = LittleFS.open(log.fullPath(), "r");
    if (!f) {
      Serial.println("  (Could not open file)");
      continue;
    }

    while (f.available()) {
      String line = f.readStringUntil('\n');
      Serial.println(line);
    }

    f.close();
  }
}

const char* LoggingManager::root_path_ = "/logs/fdl";

}  // namespace inamata

#endif