// test_clock.h — controllable backing store for the native millis() shim.
//
// Native tests drive time deterministically: set or advance the fake clock and
// every millis() call in the code under test observes the new value with no
// real waiting. Mirrors the HyphenConnect harness so the two repos share idioms.
#pragma once

namespace hyphen_test_clock {
inline unsigned long g_millis = 0;
}

inline void setMillis(unsigned long v) {
  hyphen_test_clock::g_millis = v;
}

inline void advanceMillis(unsigned long delta) {
  hyphen_test_clock::g_millis += delta;
}

inline unsigned long nowMillis() {
  return hyphen_test_clock::g_millis;
}
