#pragma once

#include <Wire.h>
#include <Adafruit_INA219.h>
#include "persistence.h"
enum GaugeType
{
    SOL = 0x40,
    VCELL = 0x41,
};

class FuelGaugeClass
{
public:
    FuelGaugeClass();

    // Call once at startup
    void init();

    // Get per-cell voltage (just battery)
    float getVCell();

    // Get overall SoC (%) between floor and observed max
    float getNormalizedSoC();

    // Optionally override default floor/full per-cell voltages
    void setCellVoltageRange(float minCellV, float maxCellV);

    float getShuntVoltage_mV();
    float getCurrent_mA();
    float getPower_mW();
    float getSolarVCell();
    float getSolarShuntVoltage_mV();
    float getSolarCurrent_mA();
    float getSolarPower_mW();

private:
    // Detect how many cells in series (1–3)
    uint8_t _cellCount = 0; // cached cell count
    uint8_t _countTick = 0;
    uint8_t _detectCellCount(float packV);
    float simDelta = 0.2f;
    float getSimulatedMaxVoltage(uint8_t cellCount);

    // Load / seed calibration for observed max
    void _loadCalibration(uint8_t cellCount);

    // Update observed max if we ever see a higher voltage
    void _updateCalibration(float packV, uint8_t cellCount);

    // Storage key based on cell count
    char _storageKey[20];

    // INA219 instances for battery (and solar if you keep that)
    Adafruit_INA219 ina219_battery;
    Adafruit_INA219 ina219_solar; // if still needed
    float lastread = -1;
    float lastSmoothed = 0; // last smoothed value
    unsigned long readTime = 0;
    float normalizePercentage(float);

    Persistence persist;          // your flash-backed key/value wrapper
    float _maxVoltage;            // highest pack voltage seen
    float _minCellVoltage = 3.0f; // default cutoff per cell
    float _maxCellVoltage = 4.2f; // default full per cell
};

// class FuelGaugeClass
// {

// private:
//     Persistence persist;
//     GaugeType defaultRead = GaugeType::VCELL;
//     // Create two INA219 instances
//     Adafruit_INA219 ina219_battery;
//     Adafruit_INA219 ina219_solar;

//     /** How many 18650s in series (1–3), inferred from voltage */
//     uint8_t _detectCellCount(float v);

//     /** Highest-seen voltage for this cell-count */
//     float _maxVoltage = 0;

//     /** Persistence key, e.g. "batt_max_v_2S" */
//     char _storageKey[20];

//     /** Load or initialize calibration based on cell-count */
//     void _loadCalibration(uint8_t cellCount);

//     /** If this reading exceeds stored max, update Flash */
//     void _updateCalibration(float v);

// public:
//     FuelGaugeClass();
//     void init();

//     float getVCell();
//     float getShuntVoltage_mV();
//     float getCurrent_mA();
//     float getPower_mW();

//     float getSolarVCell();
//     float getSolarShuntVoltage_mV();
//     float getSolarCurrent_mA();
//     float getSolarPower_mW();
//     float getNormalizedSoC();
// };
// #endif