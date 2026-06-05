// Native tests for the sample-reduction helpers (src/resources/utils/aggregation.h).
//
// These cover the pure core of the FuelGauge fix: getVCell() now samples the
// INA219 N times and reduces with median() instead of the old dead `i < 0` loop
// that returned a single noisy reading. The hardware read stays on-device; the
// reduction math is locked in here.
#include <unity.h>

#include "resources/utils/aggregation.h"

using hyphen::agg::maximum;
using hyphen::agg::median;

void setUp() {}
void tearDown() {}

void test_median_odd_count() {
  float v[] = {3.7f, 3.9f, 3.8f};
  TEST_ASSERT_FLOAT_WITHIN(0.0001f, 3.8f, median(v, 3));
}

void test_median_even_count_averages_middle_pair() {
  float v[] = {4.0f, 3.0f, 1.0f, 2.0f};  // sorted: 1,2,3,4 -> (2+3)/2
  TEST_ASSERT_FLOAT_WITHIN(0.0001f, 2.5f, median(v, 4));
}

void test_median_rejects_single_transient_spike() {
  // A lone spike must not pull the result up the way max() would.
  float v[] = {3.70f, 3.71f, 4.90f, 3.69f, 3.72f};  // sorted middle = 3.71
  TEST_ASSERT_FLOAT_WITHIN(0.0001f, 3.71f, median(v, 5));
  // max() would have reported the spike:
  TEST_ASSERT_FLOAT_WITHIN(0.0001f, 4.90f, maximum(v, 5));
}

void test_median_does_not_mutate_input() {
  float v[] = {3.0f, 1.0f, 2.0f};
  median(v, 3);
  TEST_ASSERT_EQUAL_FLOAT(3.0f, v[0]);
  TEST_ASSERT_EQUAL_FLOAT(1.0f, v[1]);
  TEST_ASSERT_EQUAL_FLOAT(2.0f, v[2]);
}

void test_empty_and_null_are_safe() {
  TEST_ASSERT_EQUAL_FLOAT(0.0f, median(nullptr, 0));
  float v[] = {1.0f};
  TEST_ASSERT_EQUAL_FLOAT(0.0f, median(v, 0));
  TEST_ASSERT_EQUAL_FLOAT(0.0f, maximum(nullptr, 5));
}

void test_single_sample() {
  float v[] = {3.85f};
  TEST_ASSERT_FLOAT_WITHIN(0.0001f, 3.85f, median(v, 1));
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_median_odd_count);
  RUN_TEST(test_median_even_count_averages_middle_pair);
  RUN_TEST(test_median_rejects_single_transient_spike);
  RUN_TEST(test_median_does_not_mutate_input);
  RUN_TEST(test_empty_and_null_are_safe);
  RUN_TEST(test_single_sample);
  return UNITY_END();
}
