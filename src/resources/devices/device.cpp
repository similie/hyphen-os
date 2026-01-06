#include "device.h"
/**
 *
 * This is the primary api for the device classes. All public attributes must be inherited by the
 * device object, even of they provide no operational function to the working of the object. Just
 * leave the function blank as below.
 *
 */

/**
 * @deconstructor
 */
Device::~Device()
{
}

/**
 * @constructor
 */
Device::Device(Bootstrap *boots)
{
}

/**
 * @constructor
 */
Device::Device()
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
void Device::publish(JsonObject &writer, uint8_t attempt_count, const String &payloadId)
{
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
void Device::restoreDefaults()
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
String Device::name()
{
    return "";
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
uint8_t Device::paramCount()
{
    return 0;
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
uint8_t Device::maintenanceCount()
{
    return 0;
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
void Device::read()
{
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
void Device::loop()
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
void Device::clear()
{
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
void Device::print()
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
void Device::init()
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
size_t Device::buffSize()
{
    return 300;
}
