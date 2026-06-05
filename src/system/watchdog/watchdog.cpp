#include "system/watchdog.h"
#include "resources/utils/watchdog_policy.h"

WatchdogClass::WatchdogClass(uint32_t periodMs, uint32_t pulseMs)
    : _periodTicks(pdMS_TO_TICKS(periodMs)), _pulseTicks(pdMS_TO_TICKS(pulseMs)), _taskHandle(nullptr)
{
    pinMode(_dogPin, OUTPUT);
    digitalWrite(_dogPin, HIGH); // Ensure the watchdog starts in a safe state
    this->periodMs = periodMs;
}

// Create the single feeder task once. Idempotent: automatic() and start() both
// call this, but only the first creates the task — there is no stop/recreate
// handoff (the old design's race window where neither source pulsed the pin).
void WatchdogClass::ensureTask()
{
    if (_taskHandle != nullptr)
    {
        return;
    }
    xTaskCreatePinnedToCore(
        _taskFunc,
        "WDT_Pet",
        2048,
        this,
        tskIDLE_PRIORITY + 1,
        &_taskHandle,
        1);
}

void WatchdogClass::automatic()
{
    // Boot / bootstrapping: feed unconditionally until start() arms gating.
    _armed = false;
    _lastHeartbeatMs = millis();
    ensureTask();
}

void WatchdogClass::start()
{
    // Arm liveness gating. Seed the heartbeat so the main loop gets a full
    // liveness window to send its first one before the feeder could give up.
    _lastHeartbeatMs = millis();
    _armed = true;
    ensureTask(); // safe if automatic() already started it
}

void WatchdogClass::loop()
{
    // Non-blocking: just record that the main loop is alive. The feeder task
    // (not this call) drives the pin, so the main loop is never stalled by a
    // pulse the way the previous loop()-pulses-inline design stalled it.
    _lastHeartbeatMs = millis();
}

void WatchdogClass::heartbeat()
{
    _lastHeartbeatMs = millis();
}

void WatchdogClass::pet()
{
    _lastHeartbeatMs = millis();
    _pulse();
}

void WatchdogClass::stop()
{
    if (_taskHandle)
    {
        vTaskDelete(_taskHandle);
        _taskHandle = nullptr;
    }
}

void WatchdogClass::_pulse()
{
    // drive HIGH for pulseTicks, then LOW (unchanged waveform: preserves the
    // edge the external watchdog chip expects)
    digitalWrite(_dogPin, HIGH);
    vTaskDelay(_pulseTicks);
    digitalWrite(_dogPin, LOW);
}

void WatchdogClass::_taskFunc(void *pv)
{
    auto *wd = static_cast<WatchdogClass *>(pv);
    while (true)
    {
        if (hyphen::watchdog::shouldFeed(wd->_armed, wd->_lastHeartbeatMs,
                                         millis(), wd->_livenessTimeoutMs))
        {
            wd->_pulse();
        }
        // If we did NOT feed, the pin stays LOW and the external watchdog will
        // reset the board once its hardware window elapses — that is the wedge
        // recovery path, and it is intentional.
        vTaskDelay(wd->_periodTicks);
    }
}
