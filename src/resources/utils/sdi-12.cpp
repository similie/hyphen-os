#include "sdi-12.h"

/**
 * @description
 *
 * Works the SDI-12 devices. The SDI-12 protocol is a one-wire, asynchronous,
 * serial communication protocol that was developed for intelligent sensors
 * that typically monitor environmental data.
 * The protocol is designed to be simple, robust, and easy to implement.
 * The SDI-12 protocol is widely used in environmental monitoring applications.
 */

/**
 * default constructor
 * @param Bootstrap boots - bootstrap object
 */
SDI12Device::SDI12Device(Bootstrap *boots)
{
    this->boots = boots;
}

/**
 * deconstructor
 */
SDI12Device::~SDI12Device()
{
}

/**
 * constructor
 * @param Bootstrap boots - bootstrap object
 * @param int identity - numerical value used to idenify the device
 */
SDI12Device::SDI12Device(Bootstrap *boots, int identity) : SDI12Device(boots)
{
    this->sendIdentity = identity;
}

/**
 * default constructor
 * @param Bootstrap boots - bootstrap object
 */
SDI12Device::SDI12Device(Bootstrap *boots, SDIParamElements *elements) : SDI12Device(boots)
{
    this->childElements = elements;
}

/**
 * constructor
 * @param Bootstrap boots - bootstrap object
 * @param int identity - numerical value used to idenify the device
 */
SDI12Device::SDI12Device(Bootstrap *boots, int identity, SDIParamElements *elements) : SDI12Device(boots, elements)
{
    this->sendIdentity = identity;
}

void SDI12Device::setElements(SDIParamElements *elements)
{
    this->childElements = elements;
}

/**
 * @public
 *
 * name
 *
 * Returns the device name
 *
 * @return String
 */
String SDI12Device::name()
{
    return getElements()->getDeviceName();
}

/**
 * @public
 *
 * extractValue
 *
 * Returns a float value for the specific parameter key
 *
 * @param float value[] - all the read values taken since the last publish event
 * @param size_t key - the key being selected for
 * @param size_t max - max size of the search scope
 *
 * @return float
 */
float SDI12Device::extractValue(float values[], size_t key, size_t max)
{
    return getElements()->extractValue(values, key, max);
}

/**
 * @public
 *
 * extractValue
 *
 * Overload for the above function @see extractValue(float values[], size_t key, size_t max)
 *
 * @param float value[] - all the read values taken since the last publish event
 * @param size_t key - the key being selected for
 *
 * @return float
 */
float SDI12Device::extractValue(float values[], size_t key)
{
    size_t MAX = readSize();
    return extractValue(values, key, MAX);
}

void SDI12Device::runSingleSample()
{
    if (!SINGLE_SAMPLE)
    {
        return;
    }
    readWire();
}

/**
 * @public
 *
 * publish
 *
 * Called when the system is ready to publish a data payload. This function pulls
 * together all the data that has been collected and places it in the writer object
 *
 * @param JSONBufferWriter &writer - the json writer object
 * @param uint8_t attempt_count - the number or reads attempted
 *
 * @return void
 */
void SDI12Device::publish(JsonObject &writer, uint8_t attempt_count)
{
    runSingleSample();
    size_t MAX = readSize();
    String *valuemap = getElements()->getValueMap();
    Utils::log("PUBLISHING_VALUES", "Param Count: " + String(getElements()->getTotalSize()));
    for (size_t i = 0; i < getElements()->getTotalSize(); i++)
    {
        if (i == getElements()->nullValue())
        {
            continue;
        }
        String param = valuemap[i];
        // set the param as having failed
        if (param.equals(NULL) || param.equals(" ") || param.equals(""))
        {
            maintenanceTick++;
            param = "_FAILURE_" + String(maintenanceTick);
        }
        float paramValue = extractValue(getElements()->getMappedValue(i), i, MAX);
        if (isnan(paramValue))
        {
            paramValue = NO_VALUE;
        }

        if (paramValue == NO_VALUE)
        {
            maintenanceTick++;
        }
        writer[param] = paramValue;
    }
}

/**
 * @private
 *
 * readReady
 *
 * Returns true with the read attempt is ready to move forward
 *
 * @return bool
 */
bool SDI12Device::readReady()
{
    if (!isConnected())
    {
        return false;
    }

    if (SINGLE_SAMPLE)
    {
        return true;
    }
    /**
     * We don't want the all weather system to take a reading every interval,
     * so we limit our read attempts
     */
    unsigned int size = boots->getReadTime() / 1000;
    size_t skip = utils.skipMultiple(size, boots->getMaxVal(), READ_THRESHOLD);
    return readAttempt >= skip;
}

/**
 * @private
 *
 * readSize
 *
 * Returns the numbers of reads that are available
 *
 * @return size_t
 */
size_t SDI12Device::readSize()
{

    if (SINGLE_SAMPLE)
    {
        return 1;
    }

    unsigned int size = boots->getReadTime() / 1000;
    size_t skip = utils.skipMultiple(size, boots->getMaxVal(), READ_THRESHOLD);
    size_t expand = floor(boots->getMaxVal() / skip);
    return expand;
}

Bootstrap *SDI12Device::getBoots()
{
    return boots;
}

/**
 * @public
 *
 * serialResponseIdentity
 *
 * Returns a response identity based on a given send identity
 * for serialized requests.
 *
 * @return String
 */
String SDI12Device::serialResponseIdentity()
{
    return utils.receiveDeviceId(this->sendIdentity);
}

String SDI12Device::getCmd()
{
    return utils.getConvertedAddressCmd(serialMsgStr, sendIdentity);
}

SDIParamElements *SDI12Device::getElements()
{
    return this->childElements;
}

bool SDI12Device::isConnected()
{
    if (!READ_ON_LOW_ONLY)
    {
        return true;
    }
    Serial.printf("Checking if device is connected: %s\n", String(digitalRead(DEVICE_CONNECTED_PIN) == LOW ? "YES" : "NO").c_str());
    return digitalRead(DEVICE_CONNECTED_PIN) == LOW;
}

/**
 * @private
 * @brief
 *
 * getWire
 *
 * pulls the wire for a given content
 *
 * @return String
 */
String SDI12Device::getWire(String content)
{
    manager.sendCommand("0R0!");
    delay(SDI12_WAIT_READ); // wait a while for a response
    return readSDI();
}

String SDI12Device::readSDI()
{
    String response = "";
    for (u_int8_t i = 0; i < READ_FAILOVER_ATTEMPTS; i++)
    {
        while (manager.available())
        {
            char c = manager.read();
            response += String(c);
        }
        delay(50);
    }
    return response;
}

/**
 * @public
 *
 * read
 *
 * Called by the device manager when a read request is required
 *
 * @return void
 */
void SDI12Device::readWire()
{
    readAttempt++;
    // Serial.printf("Read attempt: %d\n", readAttempt);
    if (!readReady())
    {
        return;
    }
    readAttempt = 0;
    // Serial.printf("Reading wire for %s with cmd %s\n", name().c_str(), getCmd().c_str());
    String response = getWire(getCmd());
    Utils::log("SDI12_RESPONSE", response);
    parseSerial(response);
}

/**
 * @public
 *
 * read
 *
 * Called by the device manager when a read request is required
 *
 * @return void
 */
void SDI12Device::read()
{
    if (SINGLE_SAMPLE)
    {
        return;
    }
    return readWire();
}

/**
 * @public
 *
 * loop
 *
 * Cycles with the main loop
 *
 * @return void
 */
void SDI12Device::loop()
{
}

/**
 * @public
 *
 * clear
 *
 * Clears any data stored for reads
 *
 * @return void
 */
void SDI12Device::clear()
{
    for (size_t i = 0; i < getElements()->getTotalSize(); i++)
    {
        for (size_t j = 0; j < boots->getMaxVal(); j++)
        {
            getElements()->setMappedValue(NO_VALUE, i, j);
        }
    }
}

/**
 * @private
 *
 * clear
 *
 * Has a specific serial numerical identity. An ID of -1 indicates
 * There is no ID specified in the constructor.
 *
 * @return bool
 */
bool SDI12Device::hasSerialIdentity()
{
    return utils.hasSerialIdentity(this->sendIdentity);
}

/**
 * @private
 *
 * uniqueName
 *
 * What's it's unique name for cloud functions
 *
 * @return String
 */
String SDI12Device::uniqueName()
{
    if (this->hasSerialIdentity())
    {
        return this->name() + String(this->sendIdentity);
    }
    return this->name();
}
/**
 * @private
 *
 * replaceSerialResponseItem
 *
 * Strips out the serialized identity from the collected string
 *
 * @param String message - serial message
 *
 * @return bool
 */
String SDI12Device::replaceSerialResponseItem(String message)
{
    if (!hasSerialIdentity())
    {
        return message;
    }
    message.replace(serialResponseIdentity() + " ", "");
    return message;
}

/**
 * @private
 *
 * parseSerial
 *
 * Parses the serial data that converts it to actual numbers
 * used during a read event
 *
 * @param String message - serial message
 *
 * @return bool
 */
void SDI12Device::parseSerial(String ourReading)
{
    readCompile = true;
    utils.parseSerial(ourReading, getElements()->getTotalSize(), boots->getMaxVal(), getElements()->valueHold);
    readCompile = false;
}

/**
 * @public
 *
 * parseSerial
 *
 * print
 *
 * Prints all the read data
 *
 * @return void
 */
void SDI12Device::print()
{
    for (size_t i = 0; i < getElements()->getTotalSize(); i++)
    {
        for (size_t j = 0; j < readSize(); j++)
        {
            Log.info("PARAM VALUES FOR %s of iteration %d and value %0.2f", utils.stringConvert(getElements()->getValueMap()[i]), j, getElements()->getMappedValue(i, j));
        }
    }
}

// void IRAM_ATTR SDI12::handleInterrupt()
// {
//     // existing code…
//     if (_activeObject)
//         _activeObject->receiveISR();
// }

// void IRAM_ATTR SDI12::receiveISR()
// {
//     // existing code…
// }

/**
 * @public
 *
 * init
 *
 * called on the system init
 *
 *
 * @return void
 */
void SDI12Device::init()
{
    if (READ_ON_LOW_ONLY)
    {
        pinMode(DEVICE_CONNECTED_PIN, INPUT_PULLUP);
    }
    manager.start();
    // pinMode(SDI12_PIN, OUTPUT_OPEN_DRAIN);
    // digitalWrite(SDI12_PIN, HIGH);
}

/**
 * @public
 *
 * restoreDefaults
 *
 * Called by the controller when a global defaul is called.
 * Not required for this device.
 *
 * @return void
 */
void SDI12Device::restoreDefaults()
{
}

/**
 * @public
 *
 * buffSize
 *
 * Returns the expected max size for all the payload data
 *
 * @return size_t
 */
size_t SDI12Device::buffSize()
{
    return getElements()->getBuffSize(); // 600;
}

/**
 * @public
 *
 * paramCount
 *
 * Returns the number of parameters that the device will have
 *
 * @return size_t
 */
uint8_t SDI12Device::paramCount()
{
    return getElements()->getTotalSize();
}

void SDI12Device::setupCloudFunctions()
{
    Hyphen.function("setAddress" + sendIdentity, &SDI12Device::setAddress, this);
    Hyphen.variable("getDeviceAddress" + sendIdentity, &sendIdentity);
}
int SDI12Device::setAddress(String address)
{
    int val = (int)atoi(address.c_str());
    if (val == -1)
    {
        Utils::log("SDI12Device", "No identity set, cannot set address");
        return -1;
    }
    String cmd = String(sendIdentity) + String("A") + String(val) + String("!");
    String response = getWire(cmd);
    Utils::log("SDI12_RESPONSE", response);
    sendIdentity = val;
    Utils::log("SDI12Device", "Setting identity to " + String(sendIdentity));
    return val;
    // Persist.put(getIdentityKey(), sendIdentity);
}

void SDI12Device::loadAddress()
{
    int identity = -1;
    // Persist.get(getIdentityKey(), identity);
}

String SDI12Device::getIdentityKey()
{
    return DEVICE_IDENTITY_ADDRESS + String(sendIdentity);
}

String SDI12Device::serialIdentity()
{
    if (randIdentity != -1)
    {
        return String(randIdentity);
    }
    int randNumber = random(300);
    randIdentity = randNumber + 1000; // Ensure it's a 4-digit number

    return String(randIdentity);
}

/**
 * @public
 *
 * maintenanceCount
 *
 * How many sensors appear non operational. The system will use this to
 * determine if device is unplugged or there are a few busted sensors.
 *
 * @return uint8_t
 */
uint8_t SDI12Device::maintenanceCount()
{
    if (!isConnected())
    {
        Utils::log("DEVICE DISCONNECTED", "SKIPPING READ");
        return paramCount();
    }
    uint8_t maintenance = this->maintenanceTick;
    maintenanceTick = 0;
    return maintenance;
}
