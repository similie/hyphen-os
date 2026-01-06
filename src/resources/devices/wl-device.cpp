#include "wl-device.h"
/**
 * Deconstructor
 */
WlDevice::~WlDevice()
{
}

/**
 * Constructor
 *
 * @param Bootstrap boots - the bootstrap object
 */
WlDevice::WlDevice(Bootstrap *boots)
{
    this->boots = boots;
}

/**
 * Constructor
 *
 * @param Bootstrap boots - the bootstrap object
 * @param int sendIdentity - the integer the identifies
 *        the specific device instance
 */
WlDevice::WlDevice(Bootstrap *boots, int sendIdentity)
{
    this->boots = boots;
    this->sendIdentity = sendIdentity;
}

/**
 * Constructor
 *
 * @param Bootstrap boots - the bootstrap object
 * @param int sendIdentity - the integer the identifies
 *        the specific device instance
 */
WlDevice::WlDevice(Bootstrap *boots, int sendIdentity, int readPin)
{
    this->readPin = readPin;
    this->boots = boots;
    this->sendIdentity = sendIdentity;
}

/**
 * @private
 *
 * getPin
 *
 * Gets the configured pin
 *
 * @return int
 */
int WlDevice::getPin()
{
    if (this->readPin != -1)
    {
        return this->readPin;
    }
    else if (digital)
    {
        return (int)DIG_PIN;
    }
    else
    {
        return AN_PIN;
    }
}

/**
 * @private
 *
 * saveEEPROM
 *
 * @param WLStruct storage - the config payload needing storage
 *
 * @return void
 */
void WlDevice::saveEEPROM(WLStruct storage)
{
    if (saveAddressForWL == 0)
    {
        return;
    }
    storage.version = 1;
    Persist.put(saveAddressForWL, storage);
}

/**
 * @private
 *
 * setDigitalCloud
 *
 * Cloud function for setting the device on a digital or
 * analogue pin.
 *
 * @param String read - payload from the particle API
 *
 * @return int
 */
int WlDevice::setDigitalCloud(String read)
{
    int val = (int)atoi(read.c_str());
    if (val < 0 || val > 1)
    {
        return 0;
    }
    digital = (bool)val;
    char saved = WlDevice::isDigital(digital);
    WLStruct storage = WlDevice::getProm();
    storage.digital = saved;
    setPin(digital);

    if (!Utils::validConfigIdentity(storage.version))
    {
        storage.calibration = currentCalibration;
    }
    saveEEPROM(storage);
    return 1;
}

/**
 * @private
 *
 * setCalibration
 *
 * Cloud function for setting the calibration value
 * @param String read - payload from the particle API
 *
 * @return int
 */
int WlDevice::setCalibration(String read)
{
    double val = Utils::parseCloudFunctionDouble(read, uniqueName());
    if (val == 0)
    {
        return 0;
    }

    currentCalibration = val;

    WLStruct storage = getProm();
    storage.calibration = currentCalibration;
    if (!Utils::validConfigIdentity(storage.version))
    {
        storage.digital = WlDevice::isDigital(digital);
    }
    saveEEPROM(storage);
    return 1;
}

/**
 * @private
 *
 * getProm
 *
 * Returns the wl configuration structure
 *
 * @return WlDevice
 */
WLStruct WlDevice::getProm()
{
    WLStruct prom;
    Persist.get(saveAddressForWL, prom);
    return prom;
}

/**
 * @private
 *
 *
 * setDigital
 *
 * We represent the digital bool as y or n so as to
 * have a value for the default
 *
 * @param bool value
 * @return char - the value for storing in EEPROM
 */
char WlDevice::setDigital(bool value)
{
    return value ? 'y' : 'n';
}

/**
 * @private
 *
 * isDigital
 *
 * Tells us if a config value is digital
 *
 * @param char value - is it digital or analogue
 *
 * @return bool
 */
bool WlDevice::isDigital(char value)
{
    if (value == 'y')
    {
        return true;
    }
    else if (value == 'n')
    {
        return false;
    }
    else
    {
        return DIGITAL_DEFAULT;
    }
}

/**
 * @private
 *
 * setPin
 *
 * Sets the pin based on the digital configuration
 *
 * @param bool digital - ?
 *
 * @return void
 */
void WlDevice::setPin(bool digital)
{
    int pin = getPin();
    if (digital)
    {
        pinMode(pin, INPUT);
    }
    else
    {
        pinMode(pin, INPUT_PULLDOWN);
    }
}

/**
 * @private
 *
 * configSetup
 *
 * Uses the EEPROM-based config and bootstraps the device
 *
 * @return void
 */
void WlDevice::configSetup()
{
    digital = WlDevice::isDigital(this->config.digital);
    const double calibration = this->config.calibration;
    currentCalibration = calibration;
    setPin(digital);
}

/**
 * @private
 *
 * restoreDefaults
 *
 * Restores the default values
 *
 * @return void
 */
bool WlDevice::hasSerialIdentity()
{
    return utils.hasSerialIdentity(this->sendIdentity);
}

/**
 * @private
 *
 * setCloudFunctions
 *
 * Binds cloud functions with the particle console
 *
 * @return void
 */
void WlDevice::setCloudFunctions()
{
    String appendage = appendIdentity();
    Hyphen.function("setDistanceAsDigital" + appendage, &WlDevice::setDigitalCloud, this);
    Hyphen.function("setDistanceCalibration" + appendage, &WlDevice::setCalibration, this);
    Hyphen.variable("isDistanceDigital" + appendage, &digital);
    Hyphen.variable("getDistanceCalibration" + appendage, &currentCalibration);
}

/**
 * @private
 *
 * appendIdentity
 *
 * If there is an integer identity, it concats the id as a string
 *
 * @return String
 */
String WlDevice::appendIdentity()
{
    return this->hasSerialIdentity() ? String(this->sendIdentity) : "";
}

/**
 * @private
 *
 * uniqueName
 *
 * If there is an integer identity, it concats the id with the name, otherwise
 * it simply returns the given name
 *
 * @return String
 */
String WlDevice::uniqueName()
{
    if (this->hasSerialIdentity())
    {
        return this->name() + String(this->sendIdentity);
    }
    return this->name();
}

long WlDevice::getReadValue()
{
    long read = 0;
    if (digital)
    {
        read = pulseIn(DIG_PIN, HIGH);
    }
    else
    {
        read = analogRead(AN_PIN);
    }
    return read;
}

/**
 * @private
 *
 * readWL
 *
 *  Performs the actual work for reading the wl sensor
 *
 * @return int
 */
int WlDevice::readWL()
{
    long timeout = 1000;
    size_t doCount = 5;
    long lastTime = millis();
    long reads[doCount];

    for (size_t i = 0; i < doCount; i++)
    {
        reads[i] = NO_VALUE;
    }

    for (size_t i = 0; i < doCount; i++)
    {
        long currentTime = millis();
        long read = getReadValue();

        utils.insertValue(read, reads, doCount);
        // break off it it is taking too long
        if (currentTime - lastTime > timeout)
        {
            break;
        }
        // we pop a quick delay to let the sensor, breath a bit
        delay(50);
        lastTime = millis();
    }

    int pw = utils.getMedian(doCount, reads);
    // or we can take the middle value
    return round(pw * currentCalibration);
}

String WlDevice::getParamName(size_t index)
{
    String param = readParams[index];
    if (this->hasSerialIdentity())
    {
        return param + StringFormat("_%d", sendIdentity);
    }
    return param;
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
void WlDevice::publish(JsonObject &writer, uint8_t attempt_count, const String &payloadId)
{
    for (size_t i = 0; i < PARAM_LENGTH; i++)
    {
        String param = getParamName(i);
        int median = utils.getMedian(attempt_count, VALUE_HOLD[i]);
        if (median == 0)
        {
            maintenanceTick++;
        }

        writer[utils.stringConvert(param)] = median;
        Log.info("Param=%s has median %d", utils.stringConvert(param), median);
    }
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
String WlDevice::name()
{
    return this->deviceName;
}

/**
 * @public
 *
 * restoreDefaults
 *
 * Restores the default values
 *
 * @return void
 */
void WlDevice::restoreDefaults()
{
    digital = DIGITAL_DEFAULT;
    setPin(digital);
    currentCalibration = digital ? DEF_DISTANCE_READ_DIG_CALIBRATION : DEF_DISTANCE_READ_AN_CALIBRATION;
    this->config.calibration = currentCalibration;
    this->config.digital = digital;
    saveEEPROM(this->config);
}

/**
 * @public
 *
 * init
 *
 * Init function for device setup
 *
 * @return void
 */
void WlDevice::init()
{
    // setup
    setCloudFunctions();
    Utils::log("WL_BOOT_ADDRESS REGISTRATION", this->uniqueName());
    saveAddressForWL = boots->registerAddress(this->uniqueName(), sizeof(WLStruct));
    Utils::log("WL_BOOT_ADDRESS " + this->uniqueName(), String(saveAddressForWL));
    this->config = getProm();
    if (!Utils::validConfigIdentity(this->config.version))
    {
        restoreDefaults();
    }
    configSetup();
}

/**
 * @public
 *
 * read
 *
 * Calls the logic for performing a read event
 *
 * @return void
 */
void WlDevice::read()
{
    int read = readWL();
    Log.info("WL %d", read);
    for (size_t i = 0; i < PARAM_LENGTH; i++)
    {
        utils.insertValue(read, VALUE_HOLD[i], boots->getMaxVal());
    }
}

/**
 * @public
 *
 * loop
 *
 * No functionality required for this device
 *
 * @return void
 */
void WlDevice::loop()
{
}

/**
 * @public
 *
 * clear
 *
 * Clears the VALUE_HOLD array
 *
 * @return void
 */
void WlDevice::clear()
{
    for (size_t i = 0; i < PARAM_LENGTH; i++)
    {
        for (size_t j = 0; j < boots->getMaxVal(); j++)
        {
            VALUE_HOLD[i][j] = NO_VALUE;
        }
    }
}

/**
 * @public
 *
 * print
 *
 * Prints the VALUE_HOLD array
 *
 * @return void
 */
void WlDevice::print()
{
    for (size_t i = 0; i < PARAM_LENGTH; i++)
    {
        for (size_t j = 0; j < boots->getMaxVal(); j++)
        {
            Log.info("PARAM VALUES FOR %s of iteration %d and value %d", utils.stringConvert(readParams[i]), j, VALUE_HOLD[i][j]);
        }
    }
}

/**
 * @public
 *
 * buffSize
 *
 * How many bytes do we need for this device
 *
 * @return size_t
 */
size_t WlDevice::buffSize()
{
    return 40;
}

/**
 * @public
 *
 * paramCount
 *
 * How many parameters are there for this device
 *
 * @return uint8_t
 */
uint8_t WlDevice::paramCount()
{
    return PARAM_LENGTH;
}

/**
 * @public
 *
 * maintenanceCount
 *
 * How many parameters are sending valid data
 *
 * @return uint8_t
 */
uint8_t WlDevice::maintenanceCount()
{
    uint8_t maintenance = this->maintenanceTick;
    maintenanceTick = 0;
    return maintenance;
}
