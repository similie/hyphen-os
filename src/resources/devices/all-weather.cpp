#include "all-weather.h"
/**
 * @description
 *
 * Works the Atmos 41 all-in-one weather sensor from Meter.
 * https://www.metergroup.com/environment/products/atmos-41-weather-station/
 *
 */

AllWeather::~AllWeather()
{
    this->sdi->~SDI12Device();
}

/**
 * default constructor
 * @param Bootstrap boots - bootstrap object
 */

AllWeather::AllWeather(Bootstrap *boots)
{
    this->sdi = new SDI12Device(boots, &this->elements);
}

/**
 * constructor
 * @param Bootstrap boots - bootstrap object
 * @param int identity - numerical value used to idenify the device
 */
AllWeather::AllWeather(Bootstrap *boots, int identity)
{

    this->sdi = new SDI12Device(boots, identity, &this->elements);
}

/**
 * @public
 *
 * init
 *
 * @brief  devices startup function
 *
 * @return void
 */
void AllWeather::init()
{
    sdi->init();
}

/**
 * @public
 *
 * publish
 *
 * @brief Called during a publish event
 *
 * @return void
 */
void AllWeather::publish(JsonObject &writer, uint8_t attempt_count)
{
    return sdi->publish(writer, attempt_count);
}

/**
 * @public
 *
 * name
 *
 * @brief Returns the device name
 * @return String
 */
String AllWeather::name()
{
    return sdi->name();
}

/**
 * @public
 *
 * paramCount
 *
 * @brief Returns the number of params returned
 *
 * @return uint8_t
 */
uint8_t AllWeather::paramCount()
{
    return sdi->paramCount();
}

/**
 * @public
 *
 * maintenanceCount
 *
 * @brief Is the device functional
 *
 * @return uint8_t
 */
uint8_t AllWeather::maintenanceCount()
{
    return sdi->maintenanceCount();
}

/**
 * @public
 *
 * read
 *
 * @brief Called during a read event
 *
 * @return void
 */
void AllWeather::read()
{
    return sdi->read();
}

/**
 * @public
 *
 * loop
 *
 * @brief Called during a loop event
 *
 * @return void
 */
void AllWeather::loop()
{
    return sdi->loop();
}

/**
 * @public
 *
 * clear
 *
 * @brief Called during a clear event
 *
 * @return void
 */
void AllWeather::clear()
{
    return sdi->clear();
}

/**
 * @public
 *
 * print
 *
 * @brief Called during a print event
 *
 * @return void
 */
void AllWeather::print()
{
    return sdi->print();
}

/**
 * @public
 *
 * buffSize
 *
 * @brief Returns the payload size the device requires for sending data
 *
 * @return size_t
 */
size_t AllWeather::buffSize()
{
    return sdi->buffSize();
}

/**
 * @public
 *
 * restoreDefaults
 *
 * @brief restores the default values
 *
 */
void AllWeather::restoreDefaults()
{
    sdi->restoreDefaults();
}