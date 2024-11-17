#include "bootstrap.h"

static bool publishHeartbeat = false;
static bool readReleased = false;
static bool publishReleased = false;
static bool staticBootstrapped = false;
// for memory debugging
static uint32_t freememLast = 0;
/**
 * releaseRead
 *
 * tells the loop that a read needs to occur
 * @return void
 */
void releaseRead()
{
    readReleased = true;
}

/**
 * releasePublishRead
 *
 * tells the loop that a publish needs to occur
 * @return void
 */
void releasePublishRead()
{
    publishReleased = true;
}

/*
 * releasePublishRead
 *
 * tells the loop that a heartbeat event needs to occur
 * @return void
 */
void releaseHeartbeat()
{
    publishHeartbeat = true;
}

/**
 * printMemory
 *
 * Prints the available memory in console for device debugging
 * @return void
 */
void printMemory()
{
    // uint32_t freemem = System.freeMemory();
    // int delta = (int)freememLast - (int)freemem;
    // // Serial.println(String::format("current %lu, last %lu, delta %d", freemem, freememLast, delta));
    // Utils::log("MEMORY CHANGE", String::format("current %lu, last %lu, delta %d", freemem, freememLast, delta));
    // freememLast = freemem;
}

#ifndef TIMERBUILD
#define TIMERBUILD
// timer setup. This are the heartbeat of the system. Triggers system events
SystemTimer publishtimer(Constants::ONE_MINUTE, releasePublishRead);
SystemTimer readtimer(Constants::ONE_MINUTE, releaseRead);
SystemTimer heartBeatTimer(Constants::HEARTBEAT_TIMER, releaseHeartbeat);
// Timer memoryPrinter(10000, printMemory);
#endif

/**
 * @constructor Bootstrap
 */
Bootstrap::~Bootstrap()
{
    int interval = (int)publicationIntervalInMinutes;
    this->READ_TIMER = (MINUTE_IN_SECONDS * interval) / MAX_SEND_TIME * MILLISECOND;
    this->PUBLISH_TIMER = interval * MINUTE_IN_SECONDS * MILLISECOND;
}

/**
 * @public init
 *
 * Called during the setup of the primary application
 * @return void
 */
void Bootstrap::init()
{
    setFunctions();
    setMetaAddresses();
    pullRegistration();
    Hyphen.keepAlive(30);
    bootstrap();
    applyTimeZone();
    Hyphen.syncTime();
    Hyphen.variable("publicationInterval", &publishedInterval);
    Hyphen.variable("batterySleepThreshold", &batterySleepThresholdValue);
    Hyphen.variable("timezone", &localTimezone);
}

/**
 * @public
 *
 * storeDevice
 *
 * Received cloud function maintenance mode request
 * @param String * decvices[]
 * @return void
 */
void Bootstrap::storeDevice(String device, int index)
{
    DeviceConfig config = {1};
    if (device.equals(""))
    {
        config = {255};
    }
    Utils::machineNameDirect(device, config.device);
    uint16_t address = deviceConfigAddresses[index];
    Persist.put(address, config);
}

/**
 * @public
 *
 * strapDevices
 *
 * Received cloud function maintenance mode request
 * @param String * decvices[]
 * @return void
 */
void Bootstrap::strapDevices(String *devices)
{
    for (uint8_t i = 0; i < MAX_DEVICES; i++)
    {
        uint16_t address = deviceConfigAddresses[i];
        DeviceConfig config;
        if (!Persist.get(address, config) || !Utils::validConfigIdentity(config.version))
        {
            devices[i] = "";
            continue;
        }
        devices[i] = Utils::machineToReadableName(config.device);
    }
}

/**
 * @private setMaintenanceMode
 *
 * Received cloud function maintenance mode request
 * @param String read - the value from the cloud
 * @return int
 */
int Bootstrap::setMaintenanceMode(String read)
{
    int val = (int)atoi(read.c_str());
    if (val < 0 || val > 1)
    {
        return -1;
    }
    setMaintenance((bool)val);
    return val;
}

/*
 * @private sendBatteryValueToConfig:
 *
 * Called to set the sleeper threshold
 * @return void
 */
void Bootstrap::sendBatteryValueToConfig(double val)
{
    EpromStruct config = getsavedConfig();
    config.sleep = val;
    putSavedConfig(config);
}

/*
 * @private sendTimezoneValueToConfig:
 *
 * Called to set the timezone
 * @return void
 */
void Bootstrap::sendTimezoneValueToConfig(int val)
{
    EpromStruct config = getsavedConfig();
    config.timezone = val;
    putSavedConfig(config);
}

/*
 * @private buildSleepThreshold:
 *
 * Called to set the sleeper threshold
 * @return void
 */
void Bootstrap::buildSleepThreshold(double sleepThreshold)
{
}

/**
 * @private setBatterySleepThreshold
 *
 * Received cloud function battery threshold mode request
 * @param String read - the value from the cloud
 * @return int
 */
int Bootstrap::setBatterySleepThreshold(String read)
{
    double val = Utils::parseCloudFunctionDouble(read, "setBatterySleepThreshold");
    buildSleepThreshold(val);
    sendBatteryValueToConfig(val);
    return 1;
}

void Bootstrap::applyTimeZone()
{
    if (!validTimezone(localTimezone))
    {
        return;
    }
    Time.zone(localTimezone);
}

int Bootstrap::setTimeZone(String read)
{

    int val = (int)atoi(read.c_str());
    if (!validTimezone(val))
    {
        return -1;
    }
    localTimezone = val;
    applyTimeZone();
    sendTimezoneValueToConfig(localTimezone);
    return val;
}

bool Bootstrap::validTimezone(int val)
{
    return val >= -12 && val <= 12;
}

/**
 * @private setFunctions
 *
 * setup for cloud functions
 * @return void
 */
void Bootstrap::setFunctions()
{
    Hyphen.function("setMaintenanceMode", &Bootstrap::setMaintenanceMode, this);
    Hyphen.function("setBatterySleepThreshold", &Bootstrap::setBatterySleepThreshold, this);
    Hyphen.function("setTimezone", &Bootstrap::setTimeZone, this);
}

/**
 * @public epromSize
 *
 * the total size of eprom memory
 * @return size_t - total memory
 */
size_t Bootstrap::epromSize()
{
    return 1024;
}

/**
 * @private deviceInitAddress
 *
 * this is the address to start storing device eprom data
 * @return uint16_t - first address after the bootstrap required space
 */
uint16_t Bootstrap::deviceInitAddress()
{
    return this->DEVICE_SPECIFIC_CONFIG_ADDRESS;
}

/**
 * @private exceedsMaxAddressSize
 *
 * Checks to see of the EEPROM address size is too excessive
 * MAX_EEPROM_ADDRESS 8197 MAX_U16 65535
 * @return bool
 */
bool Bootstrap::doesNotExceedsMaxAddressSize(uint16_t address)
{
    return address >= deviceInitAddress() && address < Bootstrap::epromSize();
}

/**
 * @private collectDevices
 *
 * Pulls all devices registered in EEPROM
 * @return void
 */
void Bootstrap::collectDevices()
{
    Utils::log("COLLECTING_REGISTERED_DEVICES", String(deviceMeta.count));
    for (uint8_t i = 0; i < MAX_DEVICES && i < this->deviceMeta.count; i++)
    {
        /*
         * Device meta address only contains the details for the device where it actually
         * stores the address pool details
         */
        uint16_t address = deviceMetaAddresses[i];
        Utils::log("DEVICE_REGISTRATION_ADDRESSES", "VALUE:: " + String(address));
        // DeviceStruct device;
        Persist.get(address, devices[i]);
        String deviceDetails = String(devices[i].version) + " " + String(devices[i].size) + " " + String(devices[i].name) + " " + String(devices[i].address);
        Utils::log("DEVICE_STORE_PULLED", deviceDetails);
    }
}

/**
 * @private maxAddressIndex
 *
 * gets max current registered
 * @return uint16_t
 */
int Bootstrap::maxAddressIndex()
{
    int maxAddressIndex = -1;
    uint16_t maxAddress = deviceInitAddress();
    for (uint8_t i = 0; i < MAX_DEVICES; i++)
    {
        uint16_t address = devices[i].address;
        Utils::log("ADDRESS_SEARCH_EVENT", StringFormat("Address, %hu Current Max, %u", address, maxAddress));
        if (this->doesNotExceedsMaxAddressSize(address))
        {
            maxAddress = address;
            maxAddressIndex = i;
        }
    }
    return maxAddressIndex;
}

/**
 * @private getDeviceByName
 *
 * adds an unregistered device to the meta structure
 * @param String name - the device name
 * @param uint16_t size - the size of data being requested
 * @return DeviceStruct - a device for registration
 */
DeviceStruct Bootstrap::getDeviceByName(String name, uint16_t size)
{
    unsigned long dName = Utils::machineName(name, true);
    // delay(10000);
    // const char * dName = name.c_str();
    for (size_t i = 0; i < MAX_DEVICES && i < this->deviceMeta.count; i++)
    {
        DeviceStruct device = this->devices[i];
        // Log.info("SEARCHING FOR REGISTERED DEVICE %lu %lu", dName, device.name);
        Utils::log("SEARCHING_FOR_REGISTERED_DEVICE", String(dName) + " " + String(device.name) + " " + String(dName == device.name));
        if (dName == device.name)
        {
            return device;
        }
    }
    // now build it
    uint16_t next = getNextDeviceAddress();
    Utils::log("THIS_IS_THE_NEXT_DEVICE_ADRESS_FOR " + name, String(next));
    DeviceStruct device = {1, size, dName, next};
    this->addNewDeviceToStructure(device);
    return device;
}

/**
 * @private
 *
 * saveDeviceMetaDetails
 *
 * Saves the device meta structure
 *
 * @return void
 */
void Bootstrap::saveDeviceMetaDetails()
{
    Persist.put(DEVICE_META_ADDRESS, this->deviceMeta);
}

/**
 * @private
 *
 * clearDeviceConfigArray
 *
 * adds an unregistered device to the meta structure
 *
 * @return void
 */
void Bootstrap::clearDeviceConfigArray()
{
    deviceMeta = {1, 0};
    saveDeviceMetaDetails();
    for (uint8_t i = 0; i < MAX_DEVICES; i++)
    {
        DeviceStruct d = {255, 0};
        devices[i] = d;
    }
}

/**
 * @public registerAddress
 *
 * adds an unregistered device to the meta structure
 * @param String name - the device name
 * @param uint16_t size - the size of data being requested
 * @return uint16_t - the predictable address for the device
 */
uint16_t Bootstrap::registerAddress(String name, uint16_t size)
{
    // delay(10000);
    DeviceStruct device = getDeviceByName(name, size);
    String deviceDetails = String(device.version) + " " + String(device.size) + " " + String(device.name) + " " + String(device.address);
    Utils::log("REGISTERED_ADDRESS_DETAILS_FOR " + name, deviceDetails);
    if (!Utils::validConfigIdentity(device.version))
    {
        // I need to know the number of devices
        uint16_t send = manualDeviceTracker;
        manualDeviceTracker = send + size + 8;
        return manualDeviceTracker;
    }
    return device.address;
}

/**
 * @private getNextDeviceAddress
 *
 * gets the next address for device registration
 * @return uint16_t
 */
uint16_t Bootstrap::getNextDeviceAddress()
{
    int mAddress = maxAddressIndex();

    if (mAddress != -1)
    {
        DeviceStruct lastDevice = devices[mAddress];
        if (Utils::validConfigIdentity(lastDevice.version))
        {
            return lastDevice.size + lastDevice.address + 8;
        }
    }
    uint16_t start = deviceInitAddress();
    return start;
}

/**
 * @private
 *
 * processRegistration
 *
 * pulls the registered devices meta details into memory
 * @return void
 */
void Bootstrap::processRegistration()
{
    Utils::log("EPROM_REGISTRATION", String(this->deviceMeta.count) + " " + String(this->deviceMeta.version));
    if (!Utils::validConfigIdentity(this->deviceMeta.version))
    {
        this->deviceMeta = {1, 0};
        saveDeviceMetaDetails();
    }

    if (this->deviceMeta.count > 0)
    {
        collectDevices();
    }
}

/**
 * @private setMetaAddresses
 *
 * Puts all the potential addresses into memory for
 * dynamic allocation
 * @return void
 */
void Bootstrap::setMetaAddresses()
{
    // delay(3000);
    uint16_t size = sizeof(DeviceStruct);
    uint16_t sizeConf = sizeof(DeviceConfig);
    for (uint8_t i = 0; i < MAX_DEVICES; i++)
    {
        deviceMetaAddresses[i] = DEVICE_CONFIG_STORAGE_META_ADDRESS + (size * i) + i;
        deviceConfigAddresses[i] = DEVICE_HOLD_ADDRESS + (sizeConf * i) + i;
        Utils::log("DEVICE_CONFIGURATION_ADDRESS, index " + String(i), "META ADDRESS " + String(deviceMetaAddresses[i]) + " DEVICE ADDRESS " + String(deviceConfigAddresses[i]));
    }
}

/**
 * @private pullRegistration
 *
 * wrapper function to get pull the device meta details into memory
 * @return void
 */
void Bootstrap::pullRegistration()
{
    // delay(1000);
    Persist.get(DEVICE_META_ADDRESS, this->deviceMeta);
    processRegistration();
}

/**
 * @private addNewDeviceToStructure
 *
 * adds an unregistered device to the meta structure
 * @param DeviceStruct device - the device for registration
 * @return void
 */
void Bootstrap::addNewDeviceToStructure(DeviceStruct device)
{
    if (device.address > this->epromSize())
    {
        Utils::log("ERROR_ADDING_DEVICE: Address_EXCEEDED", String(device.address));
        return;
    }
    uint16_t address = deviceMetaAddresses[deviceMeta.count];
    // now add based on to the next index
    Utils::log("DEVICE_IS_BEING_ADDED", StringFormat("Storing to %hu, At index, %u, With Version %u, And machine name %u", address, deviceMeta.count, device.version, device.name));
    devices[deviceMeta.count] = device;
    Persist.put(address, device);
    this->deviceMeta.count++;
    saveDeviceMetaDetails();
}

/**
 * @public hasMaintenance
 *
 * if the system is in maintenance
 * @return bool - true if in this mode
 */
bool Bootstrap::hasMaintenance()
{
    return this->maintenaceMode;
}

/**
 * @public setMaintenance
 *
 * Sets the system in maintenance mode
 * @return void
 */
void Bootstrap::setMaintenance(bool maintain)
{
    this->maintenaceMode = maintain;
}

/**
 * @public publishTimerFunc
 *
 * Sends back if a publish event is available
 * @return bool - if ready
 */
bool Bootstrap::publishTimerFunc()
{
    return publishReleased;
}

/**
 * @public heatbeatTimerFunc
 *
 * Sends back if a heartbeat event is available
 * @return bool - if ready
 */
bool Bootstrap::heartbeatTimerFunc()
{
    return publishHeartbeat;
}

/**
 * @public readTimerFun
 *
 * Sends back if a read event is available
 * @return bool - if ready
 */
bool Bootstrap::readTimerFun()
{
    return readReleased;
}

/**
 * @public setPublishTimer
 *
 * Setter for the publish event
 * @param bool - sets a publish event
 * @return void
 */
void Bootstrap::setPublishTimer(bool time)
{
    publishReleased = time;
}

/**
 * @public setHeatbeatTimer
 *
 * Setter for the heartbeat event
 * @param bool - sets a publish event
 * @return void
 */
void Bootstrap::setHeartbeatTimer(bool time)
{
    publishHeartbeat = time;
}

/**
 * @public setReadTimer
 *
 * Setter for the read event
 * @param bool - sets a publish event
 * @return void
 */
void Bootstrap::setReadTimer(bool time)
{
    readReleased = time;
}

/**
 * @public isStrapped
 *
 * is the system fully bootstrapped
 * @return bool
 */
bool Bootstrap::isStrapped()
{
    return this->bootstrapped;
}

/**
 * @public getMaxVal
 *
 * gets the maximum number or reads we store in memory
 * @return size_t
 */
size_t Bootstrap::getMaxVal()
{
    return this->MAX_VALUE_THRESHOLD;
}

/**
 * @public getPublishTime
 *
 * How often in minutes do we publish
 * @return unsigned int
 */
unsigned int Bootstrap::getPublishTime()
{
    return this->PUBLISH_TIMER;
}

/**
 * @public getReadTime
 *
 * How often are we goting read
 * @return unsigned int
 */
unsigned int Bootstrap::getReadTime()
{
    return this->READ_TIMER;
}

/**
 * @public
 *
 * resumePublication
 *
 * Stops the timers
 * @return void
 */
void Bootstrap::haultPublication()
{
    if (readtimer.isActive())
    {
        readtimer.stop();
        readtimer.reset();
    }
    if (publishtimer.isActive())
    {
        publishtimer.stop();
        publishtimer.reset();
    }
}

/**
 * @public
 *
 * resumePublication
 *
 * Starts the timers
 * @return void
 */
void Bootstrap::resumePublication()
{
    readtimer.start();
    publishtimer.start();
}

/**
 * @public
 *
 * buildSendInterval
 *
 * Chages the interval that the system sends back data and adjusts the read
 * interval accoringly.
 * @param int interval - the int value for publishing
 * @return void
 */
void Bootstrap::buildSendInterval(int interval)
{
    strappingTimers = true;
    publicationIntervalInMinutes = (uint8_t)interval;
    publishedInterval = interval;
    this->READ_TIMER = (unsigned int)(MINUTE_IN_SECONDS * publicationIntervalInMinutes) / MAX_SEND_TIME * MILLISECOND;
    this->PUBLISH_TIMER = (unsigned int)(publicationIntervalInMinutes * MINUTE_IN_SECONDS * MILLISECOND);
    haultPublication();
    publishtimer.changePeriod(PUBLISH_TIMER);
    readtimer.changePeriod(READ_TIMER);
    resumePublication();
    EpromStruct config = getsavedConfig();
    config.pub = publicationIntervalInMinutes;
    putSavedConfig(config);
    strappingTimers = false;
}

/**
 * @private getsavedConfig
 *
 * Returns the configuration stored in EEPROM
 * @return EpromStruct
 */
EpromStruct Bootstrap::getsavedConfig()
{
    EpromStruct values;
    Persist.get((uint16_t)EPROM_ADDRESS, values);
    if (values.version != 1)
    {
        EpromStruct defObject = {1, 1, TIMEZONE, 0};
        values = defObject;
    }
    return values;
}

/*
 * @private putSavedConfig:
 *
 * Stores our config struct
 * @return void
 */
void Bootstrap::putSavedConfig(EpromStruct config)
{
    config.version = 1;
    Log.info("PUTTING version %d publish: %d", config.version, config.pub);
    Persist.put((uint16_t)EPROM_ADDRESS, config);
}

/*
 * @private bootstrap:
 *
 * Called when the system bootstraps. It pulls config,
 * @return void
 */
void Bootstrap::bootstrap()
{
    EpromStruct values = getsavedConfig();
    Log.info("BOOTSTRAPPING version %d publish: %u", values.version, values.pub);
    // a default version is 2 or 0 when instantiated.
    buildSendInterval((int)values.pub);
    buildSleepThreshold(values.sleep);
    if (Utils::validConfigIdentity(values.version) && validTimezone(values.timezone))
    {
        localTimezone = values.timezone;
        applyTimeZone();
    }
    bootstrapped = true;
    staticBootstrapped = true;
}

/*
 * restoreDefaults
 *
 * cloud function that clears all config values
 * @return void
 */
void Bootstrap::restoreDefaults()
{
    // Persist.clear();
    buildSendInterval(DEFAULT_PUB_INTERVAL);
    EpromStruct defObject = {1, publicationIntervalInMinutes, TIMEZONE, 0};
    putSavedConfig(defObject);
}

/*
 * @public timers
 *
 * just validate our times are constantly active in the main loop
 * @return void
 */
void Bootstrap::timers()
{
    if (strappingTimers)
    {
        return;
    }

    if (!readtimer.isActive())
    {
        readtimer.start();
    }

    if (!publishtimer.isActive())
    {
        publishtimer.start();
    }

    if (!heartBeatTimer.isActive())
    {
        heartBeatTimer.start();
    }
}
/**
 * @public
 *
 * getProcessorName
 *
 * I ping pong event will announce the processors name
 *
 * @return String
 *
 * */
String Bootstrap::getProcessorName()
{
    return processorName;
}

/**
 * @brief removes the \n character from the end of a string
 *
 */
String Bootstrap::removeNewLine(String value)
{
    return Utils::removeNewLine(value);
}
