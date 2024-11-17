#include "Hyphen.h"

/*
Sleep class monitors voltage and can place the device to sleep if
a threshold is crossed.
*/

#ifndef system_battery_h
#define system_battery_h

class SystemBattery
{
private:
#if PLATFORM_ID == 13
    PMIC pmic;
#endif
    void disableCharging();
    const unsigned long POWER_CHECK_INTERVAL_MS = 1000;
    unsigned long lastPowerCheck = 0;

public:
    ~SystemBattery();
    SystemBattery();
    void setup();
    void loop();
};

#endif