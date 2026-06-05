// timing.h — dependency-free, rollover-safe timing helpers.
//
// These are pure functions of plain integers (no Arduino, no millis() inside),
// so they compile and unit-test on the host with zero shims. Firmware call
// sites read the clock themselves and pass it in, e.g.:
//
//     uint32_t start = millis();
//     if (hyphen::timing::timedOut(start, millis(), 10000)) { ... }
//
// Why this exists: several hand-rolled timeout checks in the codebase used
// `(millis() - timeout) < start`, which underflows in unsigned arithmetic and
// returns the wrong answer during the first `timeout` ms after boot and around
// the ~49.7-day millis() rollover. `timedOut()` is the single correct check.
#pragma once

#include <stdint.h>

namespace hyphen {
namespace timing {

// Milliseconds elapsed from `start` to `now`. Unsigned subtraction is correct
// across a single 32-bit rollover (the standard Arduino idiom).
inline uint32_t elapsed(uint32_t start, uint32_t now) {
  return (uint32_t)(now - start);
}

// True once at least `timeout` ms have elapsed since `start`. Rollover-safe.
// A `timeout` of 0 is always considered elapsed.
inline bool timedOut(uint32_t start, uint32_t now, uint32_t timeout) {
  return elapsed(start, now) >= timeout;
}

}  // namespace timing
}  // namespace hyphen
