#pragma once
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

// External hardware-watchdog driver.
//
// A dedicated, always-on FreeRTOS task pulses the watchdog pin (GPIO 32)
// independently of the main loop. Whether it pulses on a given tick is gated by
// a liveness heartbeat that the main loop refreshes each iteration:
//
//   - automatic()  — start the feeder in "grace" mode: pulse unconditionally.
//                    Used during boot/bootstrapping, which can take a while.
//   - start()      — "arm" liveness gating: from now on the feeder pulses only
//                    while the main loop keeps calling loop()/heartbeat().
//   - loop()       — record a heartbeat (non-blocking, call once per main loop).
//   - heartbeat()  — same as loop(); call inside any legitimately long operation
//                    (e.g. an OTA download) so honest progress keeps the board
//                    alive, while a true wedge stops the heartbeat and resets.
//   - pet()        — force an immediate unconditional pulse + refresh heartbeat.
//
// If the main loop wedges for longer than the liveness timeout, the feeder stops
// pulsing and the external watchdog resets the device — the "never hang"
// guarantee. The decision logic lives in resources/utils/watchdog_policy.h and
// is unit-tested on the host.
#ifndef HYPHEN_WD_LIVENESS_TIMEOUT_MS
#define HYPHEN_WD_LIVENESS_TIMEOUT_MS 30000 // tolerate the longest legit main-loop op
#endif

class WatchdogClass
{
private:
    static void _taskFunc(void *pv);
    void _pulse();
    void ensureTask();

    uint8_t _dogPin = 32;
    uint32_t periodMs = 0;
    TickType_t _periodTicks;
    TickType_t _pulseTicks;
    TaskHandle_t _taskHandle = nullptr;

    // Shared between the main loop (writers) and the feeder task (reader).
    // 32-bit aligned scalars are atomic on the ESP32, which is sufficient here.
    volatile bool _armed = false;
    volatile uint32_t _lastHeartbeatMs = 0;
    uint32_t _livenessTimeoutMs = HYPHEN_WD_LIVENESS_TIMEOUT_MS;

public:
    WatchdogClass(uint32_t periodMs = 2000, uint32_t pulseMs = 50);
    void start();     // arm liveness gating (idempotent)
    void automatic(); // start feeder in grace mode (idempotent)
    void stop();      // stop the feeder task entirely
    void pet();       // force an unconditional pulse now
    void loop();      // record a main-loop heartbeat
    void heartbeat(); // alias of loop() for long-running sections
};
