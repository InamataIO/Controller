#include <Arduino.h>

#include <vector>

#define LOG_DIRECTORY_NAME "fdl"
#define LOG_DIRECTORY_PATH "/logs/" LOG_DIRECTORY_NAME

#define MAX_LOG_SIZE 250
#define RETENTION_DAYS 7

namespace inamata {

class LoggingManager {
 private:
  struct LogPath {
    String dateTime;
    int fileNum;
    String fullPath;
  };

  std::vector<String> logBuffer;
  int fileNumber;
  int logCount;

  static std::vector<LoggingManager::LogPath> getAllLogPaths();

  void rotateLogFile();
  void deleteOldLogs();

 public:
  static void showAllLogs();

  LoggingManager();
  void addLog(const String &event, const String &status);
  void showCurrentLog();
};
}  // namespace inamata