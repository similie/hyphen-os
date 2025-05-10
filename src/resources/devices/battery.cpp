#include "battery.h"
/**
 * @deconstructor
 */
Battery::~Battery()
{
}

/**
 * @constructor
 *
 * @parm Bootstrap * boots
 */
Battery::Battery(Bootstrap *boots)
{
}

/**
 * @constructor
 */
Battery::Battery()
{
}

/**
 * @public
 *
 * name
 *
 * Returns the device name
 * @return String
 */
String Battery::name()
{
    return deviceName;
}

/**
 * @public
 *
 * restoreDefaults
 *
 * Called when defaults should be restored
 *
 * @return void
 */
void Battery::restoreDefaults()
{
}

/**
 * @public
 *
 * publish
 *
 * Called during a publish event
 *
 * @return void
 */
void Battery::publish(JsonObject &writer, uint8_t attempt_count)
{
    writer[percentname] = round(getNormalizedSoC());
    writer[voltsname] = getAvgRead();
    writer[pow] = FuelGauge.getPower_mW();
    writer[current] = FuelGauge.getCurrent_mA();
    writer[solarVolts] = FuelGauge.getSolarVCell();
    clear();
}

float Battery::getNormalizedSoC()
{
    return FuelGauge.getNormalizedSoC();
}

inline float Battery::batteryCharge()
{
    return getNormalizedSoC();
}

float Battery::getVCell()
{
    return FuelGauge.getVCell();
}

float Battery::getAvgRead()
{
    if (readCount == 0)
    {
        return getVCell();
    }

    unsigned int averageValues = batteryValue / readCount;
    float value = averageValues / 100.0;
    char buf[10];
    dtostrf(value, 6, 2, buf);
    return atof(buf);
}

/**
 * @public
 *
 * read
 *
 * Called during a read event
 *
 * @return void
 */
void Battery::read()
{
    // readCount++;
    // float val = getVCell();
    // batteryValue += (int)(val * 100);
}

/**
 * @public
 *
 * loop
 *
 * Called during a loop event
 *
 * @return void
 */
void Battery::loop()
{
}

/**
 * @public
 *
 * clear
 *
 * Called during a clear event
 *
 * @return void
 */
void Battery::clear()
{
    readCount = 0;
    batteryValue = 0;
}

/**
 * @public
 *
 * print
 *
 * Called during a print event
 *
 * @return void
 */
void Battery::print()
{
}

/**
 * @public
 *
 * init
 *
 * Called at setup
 *
 * @return void
 */
void Battery::init()
{
}

/**
 * @public
 *
 * buffSize
 *
 * Returns the payload size the device requires for sending data
 *
 * @return size_t
 */
size_t Battery::buffSize()
{
    return 100;
}

/**
 * @public
 *
 * paramCount
 *
 * Returns the number of params returned
 *
 * @return uint8_t
 */
uint8_t Battery::paramCount()
{
    return PARAM_LENGTH;
}

/**
 * @public
 *
 * maintenanceCount
 *
 * Is the device functional
 *
 * @return uint8_t
 */
uint8_t Battery::maintenanceCount()
{
    return 0;
}
