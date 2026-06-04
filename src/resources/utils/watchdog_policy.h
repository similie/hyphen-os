// watchdog_policy.h — the pure decision behind the hardware-watchdog feeder.
//
// Extracted so the "should we keep feeding the external watchdog?" logic can be
// unit-tested on the host (see test_watchdog_policy) without FreeRTOS or GPIO.
// The feeder task calls shouldFeed() every tick; the result decides whether it
// pulses GPIO 32.
#pragma once

#include <stdint.h>

#include "resources/utils/timing.h"

namespace hyphen {
namespace watchdog {

// Should the independent feeder pulse the external watchdog this tick?
//   - Not armed (boot / grace period before the main loop is running): always
//     feed, so long bootstrapping can't reset the board.
//   - Armed: feed only while the main-loop heartbeat is fresh. If the loop has
//     been silent for longer than livenessTimeoutMs it has wedged, so stop
//     feeding and let the hardware watchdog reset the board. Rollover-safe.
inline bool shouldFeed(bool armed, uint32_t lastHeartbeat, uint32_t now,
                       uint32_t livenessTimeoutMs) {
  if (!armed) {
    return true;
  }
  return !hyphen::timing::timedOut(lastHeartbeat, now, livenessTimeoutMs);
}

}  // namespace watchdog
}  // namespace hyphen
