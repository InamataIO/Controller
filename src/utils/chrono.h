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