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

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_grace_mode_always_feeds_even_when_stale);
  RUN_TEST(test_armed_feeds_while_heartbeat_fresh);
  RUN_TEST(test_armed_stops_feeding_when_wedged);
  RUN_TEST(test_armed_boundary_is_not_fed);
  RUN_TEST(test_armed_rollover_safe);
  return UNITY_END();
}
