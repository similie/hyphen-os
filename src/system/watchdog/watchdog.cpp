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
    // Boot / bootstrapping: feed unconditionally until start() arms gating — but
    // only up to _bootGraceMs, so a hung bootstrap still trips the dog.
    _armed = false;
    _graceStartMs = millis();
    _lastHeartbeatMs = millis();
    _lastProductiveMs = millis(); // give the first publish a full productivity window
    ensureTask();
}

void WatchdogClass::productive()
{
    // End-to-end progress (a successful publish). Refreshes the productivity
    // backstop so a healthy device is never power-cycled by it.
    _lastProductiveMs = millis();
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
        hyphen::watchdog::FeedInputs in;
        in.armed = wd->_armed;
        in.now = millis();
        in.graceStart = wd->_graceStartMs;
        in.graceMaxMs = wd->_bootGraceMs;
        in.lastHeartbeat = wd->_lastHeartbeatMs;
        in.livenessTimeoutMs = wd->_livenessTimeoutMs;
        in.lastProductive = wd->_lastProductiveMs;
        in.productivityTimeoutMs = wd->_productivityTimeoutMs;
        if (hyphen::watchdog::shouldFeed(in))
        {
            wd->_pulse();
        }
        // If we did NOT feed, the pin stays LOW and the external watchdog will
        // reset the board once its hardware window elapses — that is the
        // hang/stall recovery path, and it is intentional.
        vTaskDelay(wd->_periodTicks);
    }
}
