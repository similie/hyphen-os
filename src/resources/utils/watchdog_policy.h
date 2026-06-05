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

// Richer decision used by the live feeder. Two failure classes the simple
// liveness check above can't catch:
//   - a bootstrap that hangs BEFORE the loop arms gating (grace would otherwise
//     feed forever) -> bound the grace window.
//   - "alive but unproductive": the loop keeps heartbeating but the device has
//     made no end-to-end progress (no successful publish) for a long time, e.g.
//     a wedged modem -> stop feeding so the external watchdog does a FULL
//     power-cycle (which also resets the modem, unlike a software restart).
struct FeedInputs {
  bool armed = false;
  uint32_t now = 0;
  uint32_t graceStart = 0;
  uint32_t graceMaxMs = 0;  // 0 => unbounded grace (legacy)
  uint32_t lastHeartbeat = 0;
  uint32_t livenessTimeoutMs = 0;
  uint32_t lastProductive = 0;
  uint32_t productivityTimeoutMs = 0;  // 0 => productivity backstop disabled
};

inline bool shouldFeed(const FeedInputs &in) {
  if (!in.armed) {
    // boot grace: feed unconditionally, but not forever
    if (in.graceMaxMs == 0) {
      return true;
    }
    return !hyphen::timing::timedOut(in.graceStart, in.now, in.graceMaxMs);
  }
  // armed: require a fresh main-loop heartbeat...
  if (hyphen::timing::timedOut(in.lastHeartbeat, in.now, in.livenessTimeoutMs)) {
    return false;
  }
  // ...and, if enabled, recent end-to-end progress
  if (in.productivityTimeoutMs != 0 &&
      hyphen::timing::timedOut(in.lastProductive, in.now, in.productivityTimeoutMs)) {
    return false;
  }
  return true;
}

}  // namespace watchdog
}  // namespace hyphen
