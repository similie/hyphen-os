// Native tests for the hardware-watchdog feeder decision
// (src/resources/utils/watchdog_policy.h).
//
// The feeder task pulses GPIO 32 only when shouldFeed() is true. These lock in
// the "never hang" contract: feed during boot, feed while the main loop is
// alive, and STOP feeding (-> hardware reset) once the loop wedges past the
// liveness window. GPIO/FreeRTOS stay on-device; the decision is pure.
#include <unity.h>

#include "resources/utils/watchdog_policy.h"

using hyphen::watchdog::shouldFeed;

static const uint32_t LIVENESS = 30000;

void setUp() {}
void tearDown() {}

// Boot / grace mode: pulse unconditionally so slow bootstrapping never resets.
void test_grace_mode_always_feeds_even_when_stale() {
  TEST_ASSERT_TRUE(shouldFeed(/*armed=*/false, /*lastHb=*/0, /*now=*/999999,
                              LIVENESS));
}

void test_armed_feeds_while_heartbeat_fresh() {
  TEST_ASSERT_TRUE(shouldFeed(/*armed=*/true, /*lastHb=*/1000,
                              /*now=*/1000 + LIVENESS - 1, LIVENESS));
}

// Wedged main loop: heartbeat older than the window -> stop feeding -> reset.
void test_armed_stops_feeding_when_wedged() {
  TEST_ASSERT_FALSE(shouldFeed(/*armed=*/true, /*lastHb=*/1000,
                               /*now=*/1000 + LIVENESS + 1, LIVENESS));
}

void test_armed_boundary_is_not_fed() {
  // exactly at the timeout counts as elapsed (timedOut uses >=)
  TEST_ASSERT_FALSE(shouldFeed(/*armed=*/true, /*lastHb=*/1000,
                               /*now=*/1000 + LIVENESS, LIVENESS));
}

// A fresh heartbeat that spans the ~49.7-day millis() rollover must still feed.
void test_armed_rollover_safe() {
  uint32_t lastHb = 0xFFFFFF00u;            // just before wrap
  uint32_t now = (uint32_t)(lastHb + 5000); // 5s later, wrapped past 0
  TEST_ASSERT_TRUE(shouldFeed(/*armed=*/true, lastHb, now, LIVENESS));
}

// --- richer FeedInputs decision: bounded boot grace + productivity backstop ---

using hyphen::watchdog::FeedInputs;

static const uint32_t GRACE = 120000;
static const uint32_t PRODUCTIVITY = 7200000;  // 2h

// Helper: an armed, healthy device (fresh heartbeat + fresh productivity).
static FeedInputs healthy(uint32_t now) {
  FeedInputs in;
  in.armed = true;
  in.now = now;
  in.lastHeartbeat = now;
  in.livenessTimeoutMs = LIVENESS;
  in.lastProductive = now;
  in.productivityTimeoutMs = PRODUCTIVITY;
  in.graceMaxMs = GRACE;
  return in;
}

void test_bounded_grace_feeds_within_window() {
  FeedInputs in;
  in.armed = false;
  in.graceStart = 1000;
  in.graceMaxMs = GRACE;
  in.now = 1000 + GRACE - 1;
  TEST_ASSERT_TRUE(shouldFeed(in));
}

void test_bounded_grace_stops_after_window() {
  FeedInputs in;
  in.armed = false;
  in.graceStart = 1000;
  in.graceMaxMs = GRACE;
  in.now = 1000 + GRACE + 1;  // hung bootstrap that never armed
  TEST_ASSERT_FALSE(shouldFeed(in));
}

void test_unbounded_grace_when_max_zero() {
  FeedInputs in;
  in.armed = false;
  in.graceStart = 0;
  in.graceMaxMs = 0;  // legacy: feed forever during grace
  in.now = 99999999u;
  TEST_ASSERT_TRUE(shouldFeed(in));
}

void test_productive_device_is_fed() {
  TEST_ASSERT_TRUE(shouldFeed(healthy(1000000)));
}

void test_unproductive_device_is_not_fed() {
  // loop alive (fresh heartbeat) but no successful publish for > 2h -> power-cycle
  FeedInputs in = healthy(10000000);
  in.lastProductive = in.now - (PRODUCTIVITY + 1);
  TEST_ASSERT_FALSE(shouldFeed(in));
}

void test_productivity_disabled_ignores_staleness() {
  FeedInputs in = healthy(10000000);
  in.productivityTimeoutMs = 0;               // disabled
  in.lastProductive = in.now - 99999999u;     // ancient
  TEST_ASSERT_TRUE(shouldFeed(in));
}

void test_dead_loop_not_fed_even_if_recently_productive() {
  // liveness dominates: a wedged main loop resets regardless of productivity
  FeedInputs in = healthy(10000000);
  in.lastHeartbeat = in.now - (LIVENESS + 1);
  TEST_ASSERT_FALSE(shouldFeed(in));
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_grace_mode_always_feeds_even_when_stale);
  RUN_TEST(test_armed_feeds_while_heartbeat_fresh);
  RUN_TEST(test_armed_stops_feeding_when_wedged);
  RUN_TEST(test_armed_boundary_is_not_fed);
  RUN_TEST(test_armed_rollover_safe);
  RUN_TEST(test_bounded_grace_feeds_within_window);
  RUN_TEST(test_bounded_grace_stops_after_window);
  RUN_TEST(test_unbounded_grace_when_max_zero);
  RUN_TEST(test_productive_device_is_fed);
  RUN_TEST(test_unproductive_device_is_not_fed);
  RUN_TEST(test_productivity_disabled_ignores_staleness);
  RUN_TEST(test_dead_loop_not_fed_even_if_recently_productive);
  return UNITY_END();
}
