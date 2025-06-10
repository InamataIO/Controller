#pragma once

#include <Arduino.h>

#include <vector>

namespace inamata {

class LoggingManager {
 public:
  /// Max size for all log files
  static constexpr size_t kMaxTotalLogBytes = 27 * 5000;
  /// Max log entries per file
  static constexpr uint16_t kMaxLogEntries = 500;

  /**
   * Recursively prints all logs from all log files
   */
  static void showAllLogs();

  /**
   * Check if log storage exceeds max size. Delete oldest day if exceeds.
   */
  static void deleteOldLogs();

  LoggingManager();
  void addLog(const String &event);
  void showCurrentLog();

  static const char *root_path_;

 private:
  struct LogPath {
    String date_time;
    int file_num;
    const String fullPath() const {
      return String(LoggingManager::root_path_) + '/' + date_time + '/' +
             file_num + ".log";
    }
  };

  std::vector<String> log_buffer_;
  int file_number_;
  int log_count_;

  static std::vector<LoggingManager::LogPath> getAllLogPaths();

  void rotateLogFile();
};
}  // namespace inamata