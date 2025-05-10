#ifndef fuel_gauge_h
#define fuel_gauge_h

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

private:
    Persistence persist;
    GaugeType defaultRead = GaugeType::VCELL;
    // Create two INA219 instances
    Adafruit_INA219 ina219_battery;
    Adafruit_INA219 ina219_solar;

    /** How many 18650s in series (1–3), inferred from voltage */
    uint8_t _detectCellCount(float v);

    /** Highest-seen voltage for this cell-count */
    float _maxVoltage = 0;

    /** Persistence key, e.g. "batt_max_v_2S" */
    char _storageKey[20];

    /** Load or initialize calibration based on cell-count */
    void _loadCalibration(uint8_t cellCount);

    /** If this reading exceeds stored max, update Flash */
    void _updateCalibration(float v);

public:
    FuelGaugeClass();
    void init();

    float getVCell();
    float getShuntVoltage_mV();
    float getCurrent_mA();
    float getPower_mW();

    float getSolarVCell();
    float getSolarShuntVoltage_mV();
    float getSolarCurrent_mA();
    float getSolarPower_mW();
    float getNormalizedSoC();
};
#endif