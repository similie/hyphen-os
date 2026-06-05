// stress.h — field stress-test harness (DEV ONLY).
//
// Entirely compiled out unless -D HYPHEN_STRESS_TEST is set, so it can never ship
// to a field device. Provides a serial command interface + periodic telemetry for
// driving and observing chaos tests (see tools/stress/). What logs alone cannot
// show — that the watchdog actually resets a genuinely hung main loop — is proven
// here with the `w`/`W` wedge commands.
//
// Commands (single keypress over the serial monitor, 115200):
//   d  force a connection drop (connectionOff) then auto-restore after a delay
//   u  restore the connection now (connectionOn)
//   w  CLEAN wedge: stop heartbeating the watchdog (loop yields but never pets)
//        -> the independent feeder should detect the stall and reset in ~30s
//   W  HARD wedge: tight spin, no yield -> proves the feeder survives starvation
//   h  print free heap
//   s  print stats (uptime / online / connect attempts)
//   r  reboot now (ESP.restart) — for a clean power-cycle-like reset over serial
//   ?  help
//
// Optional: define HYPHEN_STRESS_AUTO_MS=<ms> to auto-toggle the connection on an
// interval (drop/reconnect cycling for heap-leak hunting). Backoff growth still
// requires real sustained network loss (pull the SIM), not this.
#pragma once
#ifdef HYPHEN_STRESS_TEST

#include <Arduino.h>
#include "Hyphen.h"
#include "system/watchdog.h"

#ifndef HYPHEN_STRESS_TELE_MS
#define HYPHEN_STRESS_TELE_MS 15000
#endif
#ifndef HYPHEN_STRESS_DROP_RESTORE_MS
#define HYPHEN_STRESS_DROP_RESTORE_MS 60000 // auto-restore a manual `d` drop after 60s
#endif

class StressHarness
{
public:
    void begin()
    {
        Serial.println("[STRESS] harness active (-D HYPHEN_STRESS_TEST). press ? for commands");
        _lastTele = millis();
#ifdef HYPHEN_STRESS_AUTO_MS
        Serial.printf("[STRESS] AUTO drop/reconnect every %d ms\n", (int)HYPHEN_STRESS_AUTO_MS);
        _lastAuto = millis();
#endif
    }

    // Call once per main-loop iteration (before manager.loop()).
    void loop()
    {
        handleSerial();
        telemetry();
        autoChaos();
        restoreIfDue();
    }

private:
    unsigned long _lastTele = 0;
    unsigned long _dropAt = 0;
    bool _dropPending = false;
#ifdef HYPHEN_STRESS_AUTO_MS
    unsigned long _lastAuto = 0;
    bool _autoDropped = false;
#endif

    void telemetry()
    {
        if (millis() - _lastTele < HYPHEN_STRESS_TELE_MS)
        {
            return;
        }
        _lastTele = millis();
        Serial.printf("[STRESS] tele uptime=%lu heap=%u online=%d attempts=%u\n",
                      (unsigned long)millis(), (unsigned)ESP.getFreeHeap(),
                      Hyphen.isOnline() ? 1 : 0, Hyphen.connectionAttempts());
    }

    void forceDrop()
    {
        Serial.println("[STRESS] inject drop (connectionOff)");
        Hyphen.connectionOff();
        _dropPending = true;
        _dropAt = millis();
    }

    void restore()
    {
        Serial.println("[STRESS] restore (connectionOn)");
        Hyphen.connectionOn();
        _dropPending = false;
    }

    void restoreIfDue()
    {
        if (_dropPending && millis() - _dropAt >= HYPHEN_STRESS_DROP_RESTORE_MS)
        {
            restore();
        }
    }

    void autoChaos()
    {
#ifdef HYPHEN_STRESS_AUTO_MS
        if (millis() - _lastAuto < HYPHEN_STRESS_AUTO_MS)
        {
            return;
        }
        _lastAuto = millis();
        if (_autoDropped)
        {
            restore();
        }
        else
        {
            Serial.println("[STRESS] AUTO inject drop");
            Hyphen.connectionOff();
        }
        _autoDropped = !_autoDropped;
#endif
    }

    void cleanWedge()
    {
        // Stop calling Watchdog.loop()/heartbeat() but keep yielding. The feeder
        // task should see the stale heartbeat and stop pulsing -> hardware reset.
        Serial.println("[STRESS] CLEAN WEDGE: main loop will stop heartbeating; expect WD reset ~30s");
        while (true)
        {
            delay(1000); // yields, but never pets the watchdog
        }
    }

    void hardWedge()
    {
        // Tight spin with no yield: proves the feeder still runs (and resets us)
        // even when the main-loop core is starved.
        Serial.println("[STRESS] HARD WEDGE: tight spin, no yield; expect WD reset ~30s");
        while (true)
        {
            // busy-loop
        }
    }

    void stats()
    {
        Serial.printf("[STRESS] stats uptime=%lu heap=%u minheap=%u online=%d attempts=%u\n",
                      (unsigned long)millis(), (unsigned)ESP.getFreeHeap(),
                      (unsigned)ESP.getMinFreeHeap(), Hyphen.isOnline() ? 1 : 0,
                      Hyphen.connectionAttempts());
    }

    void help()
    {
        Serial.println("[STRESS] d=drop u=restore w=clean-wedge W=hard-wedge h=heap s=stats r=reboot ?=help");
    }

    void handleSerial()
    {
        if (!Serial.available())
        {
            return;
        }
        int c = Serial.read();
        switch (c)
        {
        case 'd': forceDrop(); break;
        case 'u': restore(); break;
        case 'w': cleanWedge(); break;
        case 'W': hardWedge(); break;
        case 'h': Serial.printf("[STRESS] heap=%u\n", (unsigned)ESP.getFreeHeap()); break;
        case 's': stats(); break;
        case 'r': Serial.println("[STRESS] reboot"); delay(50); ESP.restart(); break;
        case '?': help(); break;
        case '\r': case '\n': break;
        default: break;
        }
    }
};

#endif // HYPHEN_STRESS_TEST
