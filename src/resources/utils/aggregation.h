// aggregation.h — dependency-free sample-reduction helpers.
//
// Pure functions over plain float arrays (no Arduino, no globals), so they
// compile and unit-test on the host. Used to turn a burst of noisy sensor/ADC
// samples into one robust value. `median` is preferred over max/mean for power
// rails because a single transient spike must not read as "battery full".
#pragma once

#include <stddef.h>

namespace hyphen {
namespace agg {

// Median of `n` samples. Sorts a local copy (n is expected small, e.g. <= 16),
// so the input is left untouched. Returns 0 for n == 0. Inputs beyond the small
// stack buffer are clamped rather than overflowing.
inline float median(const float* v, size_t n) {
  if (n == 0 || v == nullptr) {
    return 0.0f;
  }
  const size_t kMax = 32;
  if (n > kMax) {
    n = kMax;
  }
  float tmp[kMax];
  for (size_t i = 0; i < n; i++) {
    tmp[i] = v[i];
  }
  // insertion sort — fine for tiny n, no allocation
  for (size_t i = 1; i < n; i++) {
    float key = tmp[i];
    size_t j = i;
    while (j > 0 && tmp[j - 1] > key) {
      tmp[j] = tmp[j - 1];
      j--;
    }
    tmp[j] = key;
  }
  if (n & 1) {
    return tmp[n / 2];
  }
  return 0.5f * (tmp[n / 2 - 1] + tmp[n / 2]);
}

// Largest of `n` samples. Returns 0 for n == 0.
inline float maximum(const float* v, size_t n) {
  if (n == 0 || v == nullptr) {
    return 0.0f;
  }
  float m = v[0];
  for (size_t i = 1; i < n; i++) {
    if (v[i] > m) {
      m = v[i];
    }
  }
  return m;
}

}  // namespace agg
}  // namespace hyphen
