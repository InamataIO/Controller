#include "log_manager.h"

#include <Arduino.h>
#include <LittleFS.h>

#include <algorithm>
#include <vector>

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
LoggingManager::LoggingManager() : fileNumber(0), logCount(0) {
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
  if (!LittleFS.exists(LOG_DIRECTORY_PATH)) {
    if (!LittleFS.mkdir(LOG_DIRECTORY_PATH)) {
      Serial.println("Failed to create log directory");
    }
  }

  // Find the log file name to start from the next available number.
  do {
    fileNumber++;
    snprintf(buffer, sizeof(buffer), "%s/%s/%d.log", LOG_DIRECTORY_PATH,
             TimeManager::getCurrentDate().c_str(), fileNumber);

    if (fileNumber >= 255) {
      return;
    }
  } while (LittleFS.exists(buffer));

  // Delete log files older than RETENTION_DAYS.
  deleteOldLogs();
}

/**
 * @brief Rotate the log file.
 *
 * Increments the file number and resets the log count. This is called when
 * the maximum log size is reached.
 */
void LoggingManager::rotateLogFile() {
  fileNumber++;
  logCount = 0;
}

/**
 * @brief Delete old log files.
 *
 * Deletes log files older than RETENTION_DAYS. The log file name format is
 * /logs/fdl/YYYYMMDD/N.log, where N is the next available file number.
 *
 * @note The log directory name is based on the current date in the format
 * YYYYMMDD.
 */
void LoggingManager::deleteOldLogs() {
  DateTime st = TimeManager::getPastDate(RETENTION_DAYS);
  char oldDate[15];

  snprintf(oldDate, sizeof(oldDate), "%04d%02d%02d", st.year(), st.month(),
           st.day());

  String oldPath = LOG_DIRECTORY_PATH + '/' + String(oldDate);

  if (LittleFS.exists(oldPath.c_str())) {
    LittleFS.remove(oldPath.c_str());
  }
}

/**
 * @brief Add a log entry.
 *
 * Adds a log entry to the log file. The log entry format is
 * YYYY-MM-DDTHH:MM:SS - event - status.
 *
 * @param event The event to log.
 * @param status The status of the event.
 */
void LoggingManager::addLog(const String& event, const String& status) {
  char buffer[128];

  snprintf(buffer, sizeof(buffer), "%s/%s", LOG_DIRECTORY_PATH,
           TimeManager::getCurrentDate().c_str());

  String logEntry =
      TimeManager::getFormattedTime() + "," + event + "," + status;

  // Create the daily log directory if it doesn't exist (can happen if clock is
  // running closer to 24:00)
  if (!LittleFS.exists(buffer)) {
    if (!LittleFS.mkdir(buffer)) {
      Serial.println("Failed to create daily log directory:");
      Serial.println(buffer);
      return;
    }

    // Delete log files older than RETENTION_DAYS.
    deleteOldLogs();
  }

  snprintf(buffer, sizeof(buffer), "%s/%s/%d.log", LOG_DIRECTORY_PATH,
           TimeManager::getCurrentDate().c_str(), fileNumber);

  fs::File file = LittleFS.open(buffer, FILE_APPEND);
  if (!file) {
    file = LittleFS.open(buffer, FILE_WRITE);
    if (!file) {
      Serial.println("Failed to open log file");
      Serial.println(buffer);
      return;
    }
  }

#ifdef FIRE_DATA_LOGGER_EXTEND_LOGS
  Serial.print("Current Log File: ");
  Serial.println(buffer);
#endif

  file.println(logEntry.c_str());

#ifdef FIRE_DATA_LOGGER_EXTEND_LOGS
  Serial.print("Log Entry: ");
  Serial.println(logEntry);
#endif

  file.close();

  logCount++;
  if (logCount >= MAX_LOG_SIZE) {
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

  snprintf(buffer, sizeof(buffer), "%s/%s/%d.log", LOG_DIRECTORY_PATH,
           TimeManager::getCurrentDate().c_str(), fileNumber);

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
  String logRoot = String(LOG_DIRECTORY_PATH) + "/";

  File root = LittleFS.open(LOG_DIRECTORY_PATH);
  if (!root || !root.isDirectory()) {
    Serial.println("Failed to open /logs/fdl or not a directory.");
    return paths;
  }

  File dateDir;
  while ((dateDir = root.openNextFile())) {
    if (!dateDir.isDirectory()) {
      dateDir.close();
      continue;
    }

    String dateTime = dateDir.name();
    dateTime.replace(logRoot, "");

    File logFile;

    while ((logFile = dateDir.openNextFile())) {
      String name = logFile.name();
      name.replace(logRoot + dateTime + "/", "");

      if (name.endsWith(".log")) {
        int fileNum = name.substring(0, name.indexOf(".log")).toInt();
        LogPath lp;
        lp.dateTime = dateTime;
        lp.fileNum = fileNum;
        lp.fullPath = String(logRoot) + dateTime + "/" + name;
        paths.push_back(lp);
      }

      logFile.close();
    }

    dateDir.close();
  }

  // Sort: first by date descending, then by file number descending.
  std::sort(paths.begin(), paths.end(), [](const LogPath& a, const LogPath& b) {
    if (a.dateTime == b.dateTime) return a.fileNum > b.fileNum;
    return a.dateTime > b.dateTime;
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

  Serial.println("Date/Time,Action,State,Full State");

  for (const auto& log : logPaths) {
    File f = LittleFS.open(log.fullPath, "r");
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

}  // namespace inamata
