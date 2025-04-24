#include <Arduino.h>

#include <vector>

#define LOG_DIRECTORY_NAME "fdl"
#define LOG_DIRECTORY_PATH "/logs/" LOG_DIRECTORY_NAME

#define MAX_LOG_SIZE 250
#define RETENTION_DAYS 7

namespace inamata {

class LoggingManager {
 private:
  std::vector<String> logBuffer;
  int fileNumber;
  int logCount;

  void rotateLogFile();
  void deleteOldLogs();

 public:
  LoggingManager();
  void addLog(const String &event, const String &status);
  void showLogs();
};
}  // namespace inamata