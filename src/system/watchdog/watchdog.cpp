#include "system/watchdog.h"
// WatchdogClass::WatchdogClass() : WatchdogClass(2000, 50)
// {
//     // Default constructor initializes with default pin and timing
// }
WatchdogClass::WatchdogClass(uint32_t periodMs, uint32_t pulseMs) : _periodTicks(pdMS_TO_TICKS(periodMs)), _pulseTicks(pdMS_TO_TICKS(pulseMs)), _taskHandle(nullptr)
{
    pinMode(_dogPin, OUTPUT);
    digitalWrite(_dogPin, HIGH); // Ensure the watchdog starts in a safe state
}

void WatchdogClass::start()
{
    // configure pin and do an initial pet so the TPS3422 won't
    // pinMode(_dogPin, OUTPUT);
    digitalWrite(_dogPin, LOW);
    _pulse();

    // create a FreeRTOS task pinned to core 1 (optional)
    xTaskCreatePinnedToCore(
        _taskFunc,
        "WDT_Pet",
        2048,
        this,
        tskIDLE_PRIORITY + 1,
        &_taskHandle,
        1);
}

void WatchdogClass::pet()
{
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
    // drive HIGH for pulseTicks, then LOW
    digitalWrite(_dogPin, HIGH);
    vTaskDelay(_pulseTicks);
    digitalWrite(_dogPin, LOW);
}

void WatchdogClass::_taskFunc(void *pv)
{
    auto *wd = static_cast<WatchdogClass *>(pv);
    while (true)
    {
        vTaskDelay(wd->_periodTicks);
        wd->_pulse();
        Serial.println("Pettin' my dog...");
    }
}