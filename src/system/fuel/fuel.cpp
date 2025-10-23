#include "system/fuel.h"

FuelGaugeClass::FuelGaugeClass() : ina219_battery(0x41), ina219_solar(0x40)
{
}

void FuelGaugeClass::init()
{
    Wire.begin();
    ina219_battery.begin();
    ina219_solar.begin();
    persist.begin("hyphen_power");
    float v = getVCell();

    // 2) infer cell-count
    uint8_t cells = _detectCellCount(v);

    // 3) build storage key, load or seed the maxVoltage
    snprintf(_storageKey, sizeof(_storageKey),
             "maxv_%uS", unsigned(cells));
    _loadCalibration(cells);
}

uint8_t FuelGaugeClass::_detectCellCount(float v)
{

    // midpoint thresholds: 1.5×4.2=6.3, 2.5×4.2=10.5
    uint8_t count = 0;
    if (v < (_maxCellVoltage * 1.5f))
        count = 1;
    else if (v < (_maxCellVoltage * 2.5f))
        count = 2;
    else
        count = 3;

    if (_cellCount <= 0 || _cellCount == count)
    {
        // if we already have a cell count, return it
        _cellCount = count;
        return count;
    }
    // we want to make sure we are not detecting random spikes
    if (_cellCount != count && _countTick < 2)
    {
        // if we are still detecting, increment the tick
        _countTick++;
        Serial.printf("Detecting cell count: %d (tick %d)\n", count, _countTick);
        return _cellCount;
    }
    else
    {
        // if we have detected the cell count, set it
        _cellCount = count;
        Serial.printf("Detected cell count: %d\n", _cellCount);
        _countTick = 0; // reset the tick
        return count;
    }
}

float FuelGaugeClass::getSimulatedMaxVoltage(uint8_t cellCount)
{
    // theoretical max voltage for the given cell count
    return cellCount * _maxCellVoltage - (simDelta * cellCount);
}

void FuelGaugeClass::_loadCalibration(uint8_t cellCount)
{
    // try to pull last max from flash
    float stored;
    if (persist.get(_storageKey, stored))
    {

        if (stored > _maxCellVoltage * cellCount)
        {
            // if the stored value is below the minimum expected voltage, reset it
            Serial.printf("Invalid stored max voltage %.2f, resetting to theoretical max %.2f\n", stored, cellCount * _maxCellVoltage);
            stored = getSimulatedMaxVoltage(cellCount); // we want to simulate a nearly full battery
            persist.put(_storageKey, stored);
        }
        else
        {
            Serial.printf("Loaded stored max voltage: %.2f\n", stored);
        }

        _maxVoltage = stored;
    }
    else
    {
        // first run: seed to theoretical max (cellCount × 4.2V)
        _maxVoltage = getSimulatedMaxVoltage(cellCount);
        persist.put(_storageKey, _maxVoltage);
    }
}

void FuelGaugeClass::_updateCalibration(float v, uint8_t cellCount)
{
    if (v > _maxVoltage && v <= (_maxCellVoltage * cellCount))
    {
        _maxVoltage = v;
        persist.put(_storageKey, _maxVoltage);
    }
}

float FuelGaugeClass::getVCell()
{
    float maxV = ina219_battery.getBusVoltage_V();
    Serial.printf("getVCell: initial maxV = %.2f\n", maxV);
    for (uint8_t i = 0; i < 0; i++)
    {
        float v = ina219_battery.getBusVoltage_V();
        if (v > maxV)
        {
            maxV = v;
        }
        delay(10); // allow some time for the INA219 to stabilize
    }
    return maxV;
}

float FuelGaugeClass::getSolarVCell()
{
    float maxV = ina219_solar.getBusVoltage_V();
    for (uint8_t i = 0; i < 2; i++)
    {
        float v = ina219_solar.getBusVoltage_V();
        if (v > maxV)
        {
            maxV = v;
        }
        delay(10); // allow some time for the INA219 to stabilize
    }
    return maxV;
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

float FuelGaugeClass::normalizePercentage(float pct)
{
    unsigned long now = millis();
    if (lastread < 0)
    {
        lastread = pct;
        lastSmoothed = pct;
        readTime = now;
        return pct;
    }

    unsigned long timeDelta = now - readTime; // ms since last sample
    float dt = timeDelta / 1000.0f;           // convert to seconds

    // choose a time constant (how “smooth” you want it)
    // e.g. tau=5s means it takes ~5s to catch up 63% of a step
    const float tau = 5.0f;

    // α = dt / (tau + dt)  → in (0..1), small α = heavy smoothing
    float alpha = dt / (tau + dt);

    // exponential‐moving‐average
    lastSmoothed = lastSmoothed + alpha * (pct - lastSmoothed);

    // update for next time
    lastread = pct;
    readTime = now;

    return lastSmoothed;
}

// retaining for legacy use Battery supported
float FuelGaugeClass::getNormalizedSoC()
{
    // 1) read pack voltage
    float cellV = getVCell();
    uint8_t cells = _detectCellCount(cellV);
    _updateCalibration(cellV, cells);
    float floorV = cells * _minCellVoltage;
    float ceilV = cells * _maxCellVoltage;
    float delta = ceilV - _maxVoltage;
    // we are attempting to normalize the pack voltage
    float floorVAdj = floorV - abs(delta);
    float span = _maxVoltage - floorVAdj;
    Serial.printf("getNormalizedSoC: packV=%.2f, floorV=%.2f, delta=%.2f, maxV=%.2f span=%.2f, cells=%d\n", cellV, floorV, delta, _maxVoltage, span, cells);
    if (span <= 0)
        return 0.0f;

    float pct = 100 - (((_maxVoltage - cellV) / span) * 100.0f);
    if (pct < 0)
        pct = 0;
    if (pct > 100)
        pct = 100;

    return normalizePercentage(pct);
}

void FuelGaugeClass::setCellVoltageRange(float minCellV, float maxCellV)
{
    _minCellVoltage = minCellV;
    _maxCellVoltage = maxCellV;
}