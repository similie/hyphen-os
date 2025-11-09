#pragma once
#include <Ticker.h>
#include <Arduino.h>
class WatchdogClass
{
private:
    static void _taskFunc(void *pv);
    Ticker tick;
    bool _pulseReady = false;
    void _pulse();
    uint8_t _dogPin = 32;
    uint32_t periodMs = 0;
    TickType_t _periodTicks;
    TickType_t _pulseTicks;
    TaskHandle_t _taskHandle;
    static void tickHandler(WatchdogClass *instance)
    {
        instance->_pulseReady = true;
    };

public:
    WatchdogClass(uint32_t periodMs = 2000, uint32_t pulseMs = 50);
    void start();
    void automatic();
    void stop();
    void pet();
    void loop();
};