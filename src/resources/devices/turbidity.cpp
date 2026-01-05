#include "turbidity.h"
/**
 *
 * This is the primary api for the Turbidity classes. All public attributes must be inherited by the
 * Turbidity object, even of they provide no operational function to the working of the object. Just
 * leave the function blank as below.
 *
 */

/**
 * @deconstructor
 */
Turbidity::~Turbidity()
{
}

/**
 * @constructor
 */
Turbidity::Turbidity(Bootstrap *boots)
{
}

/**
 * @constructor
 */
Turbidity::Turbidity()
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
void Turbidity::publish(JsonObject &writer, uint8_t attempt_count, const String &payloadId)
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
void Turbidity::restoreDefaults()
{
}

/**
 * @public
 *
 * name
 *
 * Returns the Turbidity name
 * @return String
 */
String Turbidity::name()
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
uint8_t Turbidity::paramCount()
{
    return 0;
}

/**
 * @public
 *
 * maintenanceCount
 *
 * Is the Turbidity functional
 *
 * @return uint8_t
 */
uint8_t Turbidity::maintenanceCount()
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
void Turbidity::read()
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
void Turbidity::loop()
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
void Turbidity::clear()
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
void Turbidity::print()
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
void Turbidity::init()
{
}
/**
 * @public
 *
 * buffSize
 *
 * Returns the payload size the Turbidity requires for sending data
 *
 * @return size_t
 */
size_t Turbidity::buffSize()
{
    return 300;
}
