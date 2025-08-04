#include "utils/chrono.h"

namespace inamata {
namespace utils {

String getIsoTimestamp() {
  struct timeval tv;
  // 2025-07-07T12:31:45.123456Z needs 28 chars (incl. null terminator)
  char buffer[28];

  // Get current time with microseconds in UTC
  gettimeofday(&tv, NULL);

  // Convert seconds to struct tm in UTC
  struct tm* tm_info = gmtime(&tv.tv_sec);

  // Format time up to seconds using strftime (ISO 8601 format)
  strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%S", tm_info);

  // Append microseconds and 'Z' for UTC
  const size_t length = strlen(buffer);
  snprintf(buffer + length, sizeof(buffer) - length, ".%06ldZ", tv.tv_usec);

  return buffer;
}

}  // namespace utils
}  // namespace inamata
