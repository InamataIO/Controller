#pragma once

#include <math.h>

#include <algorithm>
#include <chrono>
#include <type_traits>

namespace inamata {
namespace utils

{
/**
 * Limit a float value between a min and max value
 *
 * \param value Value to be limitted in the range
 * \param min_value Minimum value, ignored if NAN
 * \param max_value Maximum value, ignored if NAN
 * \return Value limitted by min and max values, NAN if invalid inputs
 * \note min_value must be less than or equal to max_value
 */
float clampf(float value, const float min_value, const float max_value) {
  if (isnan(value)) {
    return NAN;
  }
  if (!isnan(min_value) && !isnan(max_value) && min_value > max_value) {
    return NAN;
  }
  if (!isnan(max_value)) {
    value = fminf(max_value, value);
  }
  if (!isnan(min_value)) {
    value = fmaxf(min_value, value);
  }
  return value;
}

template <typename Duration, typename Lower, typename Upper>
constexpr typename std::common_type<Duration, Lower, Upper>::type
clamp_duration(Duration value, Lower min, Upper max) {
  typedef
      typename std::common_type<Duration, Lower, Upper>::type CommonDuration;

  CommonDuration val = value;
  CommonDuration lo = min;
  CommonDuration hi = max;

  return std::min(hi, std::max(lo, val));
}

}  // namespace utils
}  // namespace inamata
