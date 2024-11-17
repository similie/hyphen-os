#include "Timer.h"

SystemTimer::SystemTimer(unsigned long period, void (*callback)(), bool repeat) : _period(period / 1000.0), _callback(callback), _repeat(repeat) {}

SystemTimer::SystemTimer(unsigned long period, void (*callback)()) : SystemTimer(period, callback, true) {}

SystemTimer::SystemTimer(const unsigned int period, void (*callback)()) : SystemTimer((unsigned long)period, callback, true) {};

// SystemTimer::SystemTimer(unsigned long period, void (*callback)()) : SystemTimer(period, callback, true) {}

void SystemTimer::reset()
{
    if (isActive())
    {
        stop();
    }

    if (!isActive())
    {
        start();
    }
}
void SystemTimer::start()
{
    if (_repeat)
    {
        _ticker.attach(_period, _callback);
    }
    else
    {
        _ticker.once(_period, _callback);
    }
}

bool SystemTimer::isActive()
{
    return _ticker.active();
}

void SystemTimer::stop()
{
    _ticker.detach();
}
void SystemTimer::changePeriod(unsigned long period)
{
    _period = period / 1000.0; // Convert milliseconds to seconds

    bool wasActive = _ticker.active();
    _ticker.detach(); // Stop the existing ticker

    if (wasActive)
    {
        // Restart the ticker with the new period
        if (_repeat)
        {
            _ticker.attach(_period, _callback);
        }
        else
        {
            _ticker.once(_period, _callback);
        }
    }
}