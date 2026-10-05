#pragma once

#include <Arduino.h>

#include <chrono>

namespace inamata {
namespace utils {

template <class Rep, class Period,
          class = typename std::enable_if<
              std::chrono::duration<Rep, Period>::min() <
              std::chrono::duration<Rep, Period>::zero()>::type>
constexpr inline std::chrono::duration<Rep, Period> chrono_abs(
    std::chrono::duration<Rep, Period> d) {
  return d >= d.zero() ? d : -d;
}

/**
 * Returns whether timeout has elapsed since start.
 *
 * start == time_point::min() (never started) is always treated as timed out,
 * since subtracting from it would overflow the duration's representation.
 */
template <class Clock, class Duration, class Rep, class Period>
inline bool hasTimedOut(const std::chrono::time_point<Clock, Duration>& start,
                        const std::chrono::duration<Rep, Period>& timeout) {
  if (start == std::chrono::time_point<Clock, Duration>::min()) {
    return true;
  }
  return Clock::now() - start >= timeout;
}

/**
 * Returns whether timeout has elapsed since start, using an already
 * captured now instead of a fresh Clock::now() call.
 */
template <class Clock, class Duration, class Rep, class Period>
inline bool hasTimedOut(const std::chrono::time_point<Clock, Duration>& start,
                        const std::chrono::duration<Rep, Period>& timeout,
                        const std::chrono::time_point<Clock, Duration>& now) {
  if (start == std::chrono::time_point<Clock, Duration>::min()) {
    return true;
  }
  return now - start >= timeout;
}

/**
 * Returns an ISO-8601 timestamp with microsecond precision
 *
 * Check that the time has been synced in Services::is_time_synced_. Else the
 * time returned is undefined behavior.
 *
 * \return An ISO-8601 timestamp
 */
String getIsoTimestamp();

}  // namespace utils
}  // namespace inamata