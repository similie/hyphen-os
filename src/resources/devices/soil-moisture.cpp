#include "soil-moisture.h"

/**
 * @description
 *
 * Works the the Terros 11 all-in-one weather sensor from Meter.
 * https://www.metergroup.com/environment/products/teros-11/
 *
 */

/**
 * Deconstructor
 */
SoilMoisture::~SoilMoisture()
{
    this->sdi->~SDI12Device();
}

/**
 * Constructor
 *
 * @param Bootstrap * boots - the bootstrap object
 */
SoilMoisture::SoilMoisture(Bootstrap *boots)
{
    this->elements = new SoilMoistureElements();
    this->sdi = new SDI12Device(boots, this->elements);
}

/**
 * Constructor
 *
 * @param Bootstrap * boots - the bootstrap object
 * @param int identity - the device ID that makes it unique in
 *      a multidevice environment
 */
SoilMoisture::SoilMoisture(Bootstrap *boots, int identity)
{
    this->elements = new SoilMoistureElements(identity);
    this->sdi = new SDI12Device(boots, identity, this->elements);
}

/**
 * @private
 *
 * setMoistureCalibration
 *
 * Cloud function for setting the device calibration
 *
 * @param String read - payload from the cloud
 *
 * @return int
 */
int SoilMoisture::setMoistureCalibration(String read)
{
    double val = Utils::parseCloudFunctionDouble(read, this->sdi->uniqueName());
    if (saveAddressForMoisture != -1)
    {
        VWCStruct store = {1, mineral_soil, val};
        Persist.put(saveAddressForMoisture, store);
    }
    multiple = val;
    elements->setMultiple(multiple);
    return 1;
}

/**
 * @private
 *
 * setMoistureCalibration
 *
 * Cloud function for setting the device calibration
 *
 * @param String read - payload from the cloud
 *
 * @return int
 */
int SoilMoisture::setMineralSoilCalibration(String read)
{
    int val = atoi(read.c_str());
    if (val < 0 || val > 1)
    {
        return 0;
    }
    if (saveAddressForMoisture != -1)
    {
        VWCStruct store = {1, val, multiple};
        Persist.put(saveAddressForMoisture, store);
    }
    mineral_soil = val ? true : false; //  multiple = val;
    elements->setMineralSoil(mineral_soil);
    return 1;
}

/**
 * @private
 *
 * pullEpromData
 *
 * Pulls the config data from EEPROM
 *
 * @return void
 */
void SoilMoisture::pullEpromData()
{
    VWCStruct pulled;
    if (!Persist.get(saveAddressForMoisture, pulled))
    {
        return;
    }

    if (pulled.version == 1 && !isnan(pulled.multiple))
    {
        multiple = pulled.multiple;
        elements->setMultiple(multiple);
    }
    if (pulled.version == 1 && !isnan(pulled.minerals))
    {
        mineral_soil = pulled.minerals ? true : false;
        elements->setMineralSoil(mineral_soil);
    }
    Utils::log("SOIL_MOISTURE_BOOTSTRAP_MULTIPLIER", String(mineral_soil));
}

/**
 * @private
 *
 * setDeviceAddress
 *
 * Pulls the device configuration address from bootstrap
 *
 * @return void
 */
void SoilMoisture::setDeviceAddress()
{
    saveAddressForMoisture = sdi->getBoots()->registerAddress(this->sdi->uniqueName(), sizeof(VWCStruct));
    Utils::log("SOIL_MOISTURE_BOOTSTRAP_ADDRESS", String(saveAddressForMoisture));
}

/**
 * @private
 *
 * setFunctions
 *
 * Binds the cloud functions for this device instance
 *
 * @return void
 */
void SoilMoisture::setFunctions()
{
    Hyphen.function("set" + this->sdi->uniqueName(), &SoilMoisture::setMoistureCalibration, this);
    Hyphen.function("setMineral" + this->sdi->uniqueName(), &SoilMoisture::setMineralSoilCalibration, this);
    Hyphen.variable(this->sdi->uniqueName(), &multiple);
    Hyphen.variable("mineral" + this->sdi->uniqueName(), &mineral_soil);
}

/**
 * @public
 *
 * init
 *
 * The devices startup function
 *
 * @return void
 */
void SoilMoisture::init()
{
    sdi->init();
    setDeviceAddress();
    pullEpromData();
    setFunctions();
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
void SoilMoisture::publish(JsonObject &writer, uint8_t attempt_count, const String &payloadId)
{
    return sdi->publish(writer, attempt_count);
}

/**
 * @public
 *
 * name
 *
 * Returns the device name
 * @return String
 */
String SoilMoisture::name()
{
    return sdi->name();
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
uint8_t SoilMoisture::paramCount()
{
    return sdi->paramCount();
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
uint8_t SoilMoisture::maintenanceCount()
{
    return sdi->maintenanceCount();
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
void SoilMoisture::read()
{
    return sdi->read();
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
void SoilMoisture::loop()
{
    return sdi->loop();
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
void SoilMoisture::clear()
{
    return sdi->clear();
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
void SoilMoisture::print()
{
    return sdi->print();
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
size_t SoilMoisture::buffSize()
{
    return sdi->buffSize();
}

/**
 * @public
 *
 * restoreDefaults
 *
 * Restores the default configuration
 *
 * @return void
 */
void SoilMoisture::restoreDefaults()
{
    mineral_soil = MINERAL_SOIL_DEFAULT;
    multiple = SOIL_MOISTURE_DEFAULT;
    elements->setMultiple(multiple);
    elements->setMineralSoil(mineral_soil);
    VWCStruct store = {1, mineral_soil, multiple};
    Persist.put(saveAddressForMoisture, store);
}