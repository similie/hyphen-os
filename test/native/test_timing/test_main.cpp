// Native tests for the rollover-safe timing helpers (src/resources/utils/timing.h).
//
// These lock in the fix for the broken `waitForTrue` timeout check
// (device-manager.cpp), whose `(millis() - timeout) < start` math underflowed
// and returned the wrong answer during the first `timeout` ms after boot and
// across the ~49.7-day millis() rollover. The helper is a pure function of plain
// integers, so these tests need no Arduino/clock shim at all.
#include <unity.h>

#include "resources/utils/timing.h"

using hyphen::timing::elapsed;
using hyphen::timing::timedOut;

void setUp() {}
void tearDown() {}

// --- elapsed() -------------------------------------------------------------

void test_elapsed_basic() {
  TEST_ASSERT_EQUAL_UINT32(0, elapsed(0, 0));
  TEST_ASSERT_EQUAL_UINT32(500, elapsed(1000, 1500));
}

void test_elapsed_across_rollover() {
  // start near the 32-bit ceiling, now just past wrap to 0.
  uint32_t start = 0xFFFFFFF0u;  // 16 ms before rollover
  uint32_t now = 9u;             // 10 ms after rollover
  TEST_ASSERT_EQUAL_UINT32(25u, elapsed(start, now));
}

// --- timedOut() steady state ----------------------------------------------

void test_not_timed_out_before_interval() {
  TEST_ASSERT_FALSE(timedOut(/*start=*/1000, /*now=*/1500, /*timeout=*/10000));
}

void test_timed_out_at_exact_boundary() {
  TEST_ASSERT_TRUE(timedOut(/*start=*/1000, /*now=*/11000, /*timeout=*/10000));
}

void test_timed_out_after_interval() {
  TEST_ASSERT_TRUE(timedOut(/*start=*/1000, /*now=*/99000, /*timeout=*/10000));
}

void test_zero_timeout_is_always_elapsed() {
  TEST_ASSERT_TRUE(timedOut(/*start=*/12345, /*now=*/12345, /*timeout=*/0));
}

// --- the cases the OLD code got wrong --------------------------------------

// Boot window: start=0, clock only a few ms in, timeout 10s. The old check
// `(now - timeout) < start` => (5 - 10000) underflows to a huge value, which is
// NOT < 0, so the wait fell through immediately. The correct answer is "not yet
// timed out".
void test_boot_window_not_prematurely_timed_out() {
  TEST_ASSERT_FALSE(timedOut(/*start=*/0, /*now=*/5, /*timeout=*/10000));
  TEST_ASSERT_FALSE(timedOut(/*start=*/0, /*now=*/9999, /*timeout=*/10000));
  TEST_ASSERT_TRUE(timedOut(/*start=*/0, /*now=*/10000, /*timeout=*/10000));
}

// Rollover window: a 10s timeout that begins 16 ms before the millis() wrap and
// is checked 10 ms after the wrap should NOT be considered elapsed.
void test_rollover_window_not_falsely_timed_out() {
  uint32_t start = 0xFFFFFFF0u;
  TEST_ASSERT_FALSE(timedOut(start, /*now=*/9u, /*timeout=*/10000));
  // ...and DOES elapse once 10s of (rollover-spanning) time passes.
  uint32_t now = (uint32_t)(start + 10000u);  // wraps
  TEST_ASSERT_TRUE(timedOut(start, now, /*timeout=*/10000));
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_elapsed_basic);
  RUN_TEST(test_elapsed_across_rollover);
  RUN_TEST(test_not_timed_out_before_interval);
  RUN_TEST(test_timed_out_at_exact_boundary);
  RUN_TEST(test_timed_out_after_interval);
  RUN_TEST(test_zero_timeout_is_always_elapsed);
  RUN_TEST(test_boot_window_not_prematurely_timed_out);
  RUN_TEST(test_rollover_window_not_falsely_timed_out);
  return UNITY_END();
}
