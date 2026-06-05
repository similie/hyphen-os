// Ticker.h — native no-op shim for the ESP32 Ticker. Timer-driven paths do
// nothing on their own in tests; tests drive state deterministically.
#pragma once

class Ticker {
 public:
  Ticker() = default;

  template <typename TArg>
  void attach(float, void (*)(TArg), TArg) {}
  void attach(float, void (*)()) {}

  template <typename TArg>
  void once(float, void (*)(TArg), TArg) {}
  void once(float, void (*)()) {}

  void detach() {}
  bool active() const { return false; }
};
