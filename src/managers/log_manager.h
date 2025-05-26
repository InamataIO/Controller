#pragma once

#include <Arduino.h>

#include <vector>

#define MAX_LOG_SIZE 250
#define RETENTION_DAYS 7

namespace inamata {

class LoggingManager {
 public:
  static void showAllLogs();

  LoggingManager();
  void addLog(const String &event, const String &status);
  void showCurrentLog();

  static const char *root_path_;

 private:
  struct LogPath {
    String date_time;
    int file_num;
    String full_path;
  };

  std::vector<String> log_buffer_;
  int file_number_;
  int log_count_;

  static std::vector<LoggingManager::LogPath> getAllLogPaths();

  void rotateLogFile();
  void deleteOldLogs();
};
}  // namespace inamata