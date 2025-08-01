#pragma once
#include <Arduino.h>
class WatchdogClass
{
private:
    static void _taskFunc(void *pv);
    void _pulse();
    uint8_t _dogPin = 32;
    TickType_t _periodTicks;
    TickType_t _pulseTicks;
    TaskHandle_t _taskHandle;

public:
    WatchdogClass(uint32_t periodMs = 2000, uint32_t pulseMs = 50);
    // WatchdogClass();
    void start();
    void stop();
    void pet();
};