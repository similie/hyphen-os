#include "system/fuel.h"

FuelGaugeClass::FuelGaugeClass() : ina219_battery(0x41), ina219_solar(0x40)
{
}

void FuelGaugeClass::init()
{
    Wire.begin();
    ina219_battery.begin();
    ina219_solar.begin();
    persist.begin("hyphen_power_storage");
    float v = getVCell();

    // 2) infer cell-count
    uint8_t cells = _detectCellCount(v);

    // 3) build storage key, load or seed the maxVoltage
    snprintf(_storageKey, sizeof(_storageKey),
             "batt_max_v_%uS", unsigned(cells));
    _loadCalibration(cells);
}

uint8_t FuelGaugeClass::_detectCellCount(float v)
{
    // midpoint thresholds: 1.5×4.2=6.3, 2.5×4.2=10.5
    if (v < (4.2f * 1.5f))
        return 1;
    else if (v < (4.2f * 2.5f))
        return 2;
    else
        return 3;
}

void FuelGaugeClass::_loadCalibration(uint8_t cellCount)
{
    // try to pull last max from flash
    float stored;
    if (persist.get(_storageKey, stored))
    {
        _maxVoltage = stored;
    }
    else
    {
        // first run: seed to theoretical max (cellCount × 4.2V)
        _maxVoltage = cellCount * 4.2f;
        persist.put(_storageKey, _maxVoltage);
    }
}

void FuelGaugeClass::_updateCalibration(float v)
{
    if (v > _maxVoltage)
    {
        _maxVoltage = v;
        persist.put(_storageKey, _maxVoltage);
    }
}

float FuelGaugeClass::getVCell()
{
    return ina219_battery.getBusVoltage_V();
}

float FuelGaugeClass::getSolarVCell()
{
    return ina219_solar.getBusVoltage_V();
}

float FuelGaugeClass::getShuntVoltage_mV()
{
    return ina219_battery.getShuntVoltage_mV();
}
float FuelGaugeClass::getCurrent_mA()
{
    return ina219_battery.getCurrent_mA();
}
float FuelGaugeClass::getPower_mW()
{
    return ina219_battery.getPower_mW();
}

float FuelGaugeClass::getSolarShuntVoltage_mV()
{
    return ina219_solar.getShuntVoltage_mV();
}
float FuelGaugeClass::getSolarCurrent_mA()
{
    return ina219_solar.getCurrent_mA();
}
float FuelGaugeClass::getSolarPower_mW()
{
    return ina219_solar.getPower_mW();
}

// retaining for legacy use Battery supported
float FuelGaugeClass::getNormalizedSoC()
{
    // 1) read pack voltage
    float v = getVCell();

    // 2) see if this is a new high-water mark
    _updateCalibration(v);

    // 3) SoC = (measured ÷ maxSeen) * 100, capped to 100%
    float pct = (v / _maxVoltage) * 100.0f;
    return (pct > 100.0f ? 100.0f : pct);
    // float voltage = getVCell();
    // voltage > 4.4 ? voltage / 2 : voltage;
    // const float voltages[] = {3.0, 3.2, 3.3, 3.4, 3.5, 3.6, 3.7, 3.8, 3.9, 4.0, 4.1, 4.2};
    // const float percentages[] = {0, 5, 10, 20, 30, 40, 50, 60, 70, 80, 90, 100};
    // const size_t tableSize = sizeof(voltages) / sizeof(voltages[0]);
    // // If voltage is out of bounds, clamp to min or max
    // if (voltage <= voltages[0])
    //     return percentages[0];
    // if (voltage >= voltages[tableSize - 1])
    //     return percentages[tableSize - 1];

    // // Find the range in the lookup table
    // for (size_t i = 1; i < tableSize; i++)
    // {
    //     if (voltage <= voltages[i])
    //     {
    //         // Perform linear interpolation
    //         float slope = (percentages[i] - percentages[i - 1]) / (voltages[i] - voltages[i - 1]);
    //         return percentages[i - 1] + slope * (voltage - voltages[i - 1]);
    //     }
    // }

    // return 0; // Default fallback, should not be reached
}