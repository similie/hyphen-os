#include "relay.h"
/**
 *
 * This is the primary api for the Relay classes. All public attributes must be inherited by the
 * Relay object, even of they provide no operational function to the working of the object. Just
 * leave the function blank as below.
 *
 */

/**
 * @deconstructor
 */
Relay::~Relay()
{
    init();
}

/**
 * @constructor
 */
Relay::Relay(Bootstrap *boots)
{
    this->boots = boots;
}

/**
 * @constructor
 */
Relay::Relay(Bootstrap *boots, int pin) : Relay(boots)
{
    this->pin = pin;
}

/**
 * @constructor
 */
Relay::Relay(Bootstrap *boots, int pin, bool invert) : Relay(boots, pin)
{
    this->invert = invert;
}
/**
 * @constructor
 */
Relay::Relay(Bootstrap *boots, int pin, String identity) : Relay(boots, pin)
{
    this->sendIdentity = identity;
    // setCloudFunctions();
}

/**
 * @constructor
 */
Relay::Relay(Bootstrap *boots, int pin, String identity, bool invert) : Relay(boots, pin, invert)
{
    this->sendIdentity = identity;
}

/**
 * @constructor
 */
Relay::Relay(int pin)
{
    this->pin = pin;
    init();
}

/**
 * @constructor
 */
Relay::Relay(int pin, bool invert)
{
    this->pin = pin;
    this->invert = invert;
    init();
}

bool Relay::on()
{
    digitalWrite(pin, POW_ON);
    isOn = true;
    startTime = millis();
    return true;
}

bool Relay::off()
{
    digitalWrite(pin, POW_OFF);
    isOn = false;
    startTime = 0;
    runTime = 0;
    timeRunning = 0;
    return false;
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
void Relay::publish(JsonObject &writer, uint8_t attempt_count, const String &payloadId)
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
void Relay::restoreDefaults()
{
}

/**
 * @public
 *
 * name
 *
 * Returns the Relay name
 * @return String
 */
String Relay::name()
{
    return "relay" + sendIdentity;
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
uint8_t Relay::paramCount()
{
    return 0;
}

/**
 * @public
 *
 * maintenanceCount
 *
 * Is the Relay functional
 *
 * @return uint8_t
 */
uint8_t Relay::maintenanceCount()
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
void Relay::read()
{
}

/**
 * @brief get the delta time from the start time
 *
 * @return unsigned long
 */
unsigned long Relay::getStartDelta()
{
    unsigned long currentTime = millis();
    unsigned long delta = currentTime - startTime;
    return delta / 1000;
}

/**
 * @brief checks to see if there is a runTime and turns off when it exceeds
 *
 */
void Relay::shouldToggleOff()
{
    if (!isOn || runTime == 0 || startTime == 0)
    {
        return;
    }
    timeRunning = getStartDelta();
    if (timeRunning < runTime)
    {
        return;
    }
    off();
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
void Relay::loop()
{
    shouldToggleOff();
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
void Relay::clear()
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
void Relay::print()
{
}
/**
 * @brief if inverted pulls low, otherwise high
 *
 */
void Relay::buildOutputs()
{
    POW_ON = invert ? LOW : HIGH;
    POW_OFF = invert ? HIGH : LOW;
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
void Relay::init()
{
    buildOutputs();
    pinMode(pin, OUTPUT);
    digitalWrite(pin, POW_OFF);
    setCloudFunctions();
}
/**
 * @brief Returns the identity of the relay
 *
 * @return String
 */
String Relay::appendIdentity()
{
    return this->sendIdentity;
}

/**
 * @brief for cloud functions
 *
 * @param command
 * @return int
 */
int Relay::turnOn(String command)
{
    runTime = Utils::parseCloudFunctionInteger(command, "relayTurnOn" + appendIdentity());
    Utils::log("RELAY_POWER_ON", String(runTime));
    return on();
}
/**
 * @brief for cloud functions
 *
 * @param command
 * @return int
 */
int Relay::turnOff(String command)
{
    return off();
}
/**
 * @brief cloud function to toggle relay
 *
 * @param command
 * @return int
 */
int Relay::toggle(String command)
{
    return isOn ? off() : on();
}
/**
 * @brief cloud function to invert the relay
 *
 * @param command
 * @return int
 */
int Relay::setInvert(String command)
{
    invert = Utils::parseCloudFunctionInteger(command, "relayInvert" + appendIdentity());
    init();
    return invert;
}
/**
 * @brief sets the cloud functions for the relay device
 *
 */
void Relay::setCloudFunctions()
{
    if (registered || !boots)
    {
        return;
    }

    String appendage = appendIdentity();
    Hyphen.function("turnOn" + appendage, &Relay::turnOn, this);
    Hyphen.function("turnOff" + appendage, &Relay::turnOff, this);
    Hyphen.function("toggle" + appendage, &Relay::toggle, this);
    Hyphen.function("setInvert" + appendage, &Relay::setInvert, this);
    Hyphen.variable("isOn" + appendage, &isOn);
    Hyphen.variable("relayPin" + appendage, &pin);
    Hyphen.variable("timeRunning" + appendage, &timeRunning);
    Hyphen.variable("runTime" + appendage, &runTime);
    Hyphen.variable("isInverted" + appendage, &invert);
    registered = true;
}
/**
 * @public
 *
 * buffSize
 *
 * Returns the payload size the Relay requires for sending data
 *
 * @return size_t
 */
size_t Relay::buffSize()
{
    return 0;
}
