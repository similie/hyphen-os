#include "battery.h"

SystemBattery::SystemBattery()
{
#if PLATFORM_ID == 13
    pmic.begin();
#endif
}

SystemBattery::~SystemBattery()
{
}

void SystemBattery::loop()
{
#if PLATFORM_ID == 13

    if (millis() - lastPowerCheck < POWER_CHECK_INTERVAL_MS)
    {
        return;
    }
    lastPowerCheck = millis();
    if (pmic.isPowerGood())
    {
        return;
    }

    pmic.disableBATFET();

#endif
}

void SystemBattery::setup()
{
#if PLATFORM_ID == 13
    pmic.disableCharging();
    pmic.enableBATFET();
#endif
}
