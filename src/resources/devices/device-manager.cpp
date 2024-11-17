#include "device-manager.h"
/**
 * ~DeviceManager
 *
 * Default deconstructor
 */
DeviceManager::~DeviceManager()
{
}

/**
 * DeviceManager
 *
 * Default constructor. Add your devices to the devices multi-dimensional
 * array. Generally we will only need one device array established. But there
 * are options for more. The limit is 7 devices applied to a given box.
 *
 * @param Processor * processor - the process for sending data
 */
DeviceManager::DeviceManager(LocalProcessor *processor, bool debug)
{
    // To clear Persistance. Comment this line
    // once flashed you should re-flash to avoid data loss
    // FRESH_START = true;
    if (FRESH_START)
    {
        Persist.clear();
    }

    this->blood = new HeartBeat(Hyphen.deviceID());
    // end devices
    // set storage when we have a memory card reader
    // instantiate the processor
    this->processor = processor;
    // turn on or off system logging
    Utils::setDebug(debug);

    /**
     * Our primary method of device configuration is via the particle
     * cloud using the addDevice function. However, we can also configure
     * our devices directly in the constructor by setting the deviceAggregateCounts array.
     * Normally you will leave the first dimension at ONE_I.`
     * Between the curly braces {NUM}, Set the number of devices you need to initialize.
     * The max is by default set to 7. If you want to collect multiple datasets, then set
     * another dimension. This behavior is not support through cloud configuration
     * (only a single dataset is available), but it can be set here manually.
     */
    // deviceAggregateCounts[ONE_I] = {ONE};
    // deviceAggregateCounts[ONE_I] = {FIVE}; //{FOUR}; // set the number of devices here
    // // deviceAggregateCounts[TWO_I] = {ONE};
    // // the numerical N_I values a indexes from 0, 1, 2 ... n
    // // unless others datasets are needed. Most values will only needs the
    // // ONE_I for the first dimension.
    // this->devices[ONE_I][ONE_I] = new VideoCapture(&boots);
    // // this->devices[ONE_I][ONE_I] = new RikaAirQuality(&boots, "03:CH4", 4);
    // this->devices[ONE_I][ONE_I] = new RikaAirQuality(&boots, "T;H;P;PM2;PM10", 2);
    // this->devices[ONE_I][TWO_I] = new RikaAirQuality(&boots, "CO2;;SO2;NO2", 5);
    // this->devices[ONE_I][THREE_I] = new RikaAirQuality(&boots, "03;CH4", 4);

    // // all weather
    // deviceAggregateCounts[ONE_I] = {TWO};
    // this->devices[ONE_I][ONE_I] = new AllWeather(&boots, ONE_I);
    // // battery
    // this->devices[ONE_I][TWO_I] = new Battery();

    // this->devices[ONE_I][TWO_I] = new AllWeather(&boots, ONE_I);
    // // battery
    // this->devices[ONE_I][THREE_I] = new Battery();
    // soil moisture
    // this->devices[ONE_I][THREE_I] = new SoilMoisture(&boots, TWO_I);
    // // water level
    // this->devices[ONE_I][FOUR_I] = new WlDevice(&boots, TWO_I);
    // rain gauge
    // this->devices[ONE_I][FIVE_I] = new RainGauge(boots);
}

//////////////////////////////
/// Public Functions
//////////////////////////////
/**
 * @public
 *
 * setSendInverval
 *
 * Cloud function for setting the send interval
 * @return void
 */
int DeviceManager::setSendInterval(String read)
{
    int val = (int)atoi(read.c_str());
    // we dont let allows less than one or greater than 15
    if (val < 1 || val > 15)
    {
        return 0;
    }
    Utils::log("CLOUD_REQUESTED_INTERVAL_CHANGE", "setting payload delivery for every " + String(val) + " minutes");
    buildSendInterval(val);
    return val;
}

/**
 * @public
 *
 * init
 *
 * Init function called at the device setup
 * @return void
 */
void DeviceManager::init()
{
    // apply delay to see the devices bootstrapping
    // in the serial console
    // Persist.clear();
    // delay(10000);

    Serial.println("Connecting...");
    if (!processor->init())
    {
        Serial.println("Failed to connect to the network");
        return init();
    }
    Log.noticeln("BootStrapping");
    delay(2000);
    boots.init();
    Log.noticeln("ITERATING DEVICES");
    delay(2000);
    waitForTrue(&DeviceManager::isStrapped, this, 10000);
    // // if there are already default devices, let's process
    // // their init before we run the dynamic configuration
    iterateDevices(&DeviceManager::initCallback, this);
    Log.noticeln("Strapping DEVICES");
    // delay(2000);
    strapDevices();
    Log.noticeln("Setting Counts");
    // delay(2000);
    setParamsCount();
    Log.noticeln("Setting Cloud Functions");
    // delay(2000);
    setCloudFunctions();
    Log.noticeln("Clearing Array");
    // delay(2000);
    clearArray();
}

/**
 * @public
 *
 * setReadCount
 *
 * Sets the read count
 * @param unsigned int read_count
 * @return void
 */
void DeviceManager::setReadCount(unsigned int read_count)
{
    this->read_count = read_count;
}

/**
 * @public
 *
 * recommendedMaintence
 *
 * Is maintenance mode recommended based on the timestamps
 * and the number of failed parameters? Device has neither connected
 * to the cloud nor has sensor connected.
 * @return bool - true if we should go to maintenance mode
 */
bool DeviceManager::recommendedMaintence(uint8_t damageCount)
{
    long time = Time.now();
    const long THRESHOLD = 1600000000;
    if (time < THRESHOLD)
    {
        return true;
    }
    uint8_t doubleCount = damageCount * 2;
    return doubleCount >= this->paramsCount;
}

/**
 * @public
 *
 * isNotPublishing
 *
 * It is not currently in publish mode
 * @return bool - true if a publish event can proceed
 */
bool DeviceManager::isNotPublishing()
{
    return !publishBusy;
}

/**
 * @public
 *
 * isNotReading
 *
 * It is not currently in read mode
 * @return bool - true if a read event can proceed
 */
bool DeviceManager::isNotReading()
{
    return !readBusy;
}

/**
 * @public
 *
 * clearArray
 *
 * Sends api request to clear devices storage arrays
 * @return void
 */
void DeviceManager::clearArray()
{
    iterateDevices(&DeviceManager::clearArrayCallback, this);
}

/**
 * @public
 *
 * loop
 *
 * runs off the main loop
 * @return void
 */
void DeviceManager::loop()
{
    process();
    boots.timers();
    processor->loop();
    processTimers();
    iterateDevices(&DeviceManager::loopCallback, this);
}

//////////////////////////////
/// Private Functions
//////////////////////////////
/**
 * @public
 *
 * setParamsCount
 *
 * Counts the number of params that the system is collecting from
 * all of the initialized devices
 * @return void
 */
void DeviceManager::setParamsCount()
{
    iterateDevices(&DeviceManager::setParamsCountCallback, this);
}

/**
 * @private
 *
 * process
 *
 * This waits until there is a request
 * to reboot the system
 *
 * @return void
 */
void DeviceManager::process()
{
    if (rebootEvent)
    {
        delay(1000);
        Utils::reboot();
    }
}

/**
 * @private
 *
 * processTimers
 *
 * Checks the timers to see if they are ready for the various events
 *
 * @return void
 */
void DeviceManager::processTimers()
{
    if (boots.readTimerFun())
    {
        boots.setReadTimer(false);
        read();
    }

    if (boots.publishTimerFunc())
    {
        boots.setPublishTimer(false);
        publish();
    }

    if (processor->hasHeartbeat() && boots.heartbeatTimerFunc())
    {
        boots.setHeartbeatTimer(false);
        heartbeat();
    }
}

/**
 * @private
 *
 * rebootRequest
 *
 * Cloud function that calls a reboot request
 * @param String read
 */
int DeviceManager::rebootRequest(String read)
{
    Utils::log("REBOOT_EVENT_REQUESTED", "Shutting down");
    rebootEvent = true;
    return 1;
}

/**
 * @private
 *
 * restoreDefaults
 *
 * Cloud function for resetting defaults
 *
 * @param String read
 *
 * @return void
 */
int DeviceManager::restoreDefaults(String read)
{
    Utils::log("RESTORING_DEVICE_DEFAULTS", read);
    this->processRestoreDefaults();
    return 1;
}

/**
 * @private
 *
 * restoreDefaults
 *
 * Calls the devices to restore their default values
 * should there be a request
 * @return void
 */
void DeviceManager::processRestoreDefaults()
{
    boots.restoreDefaults();
    iterateDevices(&DeviceManager::restoreDefaultsCallback, this);
}

/**
 * @private
 *
 * storePayload
 *
 * A payload need to be stored to a given memory card
 * @return void
 */
void DeviceManager::storePayload(String payload, String topic)
{
    // this->storage->storePayload(payload, topic);
}

/**
 * @private
 *
 * heartbeat
 *
 * Publishes a heartbeach payload
 * @return void
 */
void DeviceManager::heartbeat()
{
    if (processor->isConnected())
    {
        String artery = blood->pump();
        Utils::log("SENDING_HEARTBEAT", artery);
        processor->publish(processor->getHeartbeatTopic(), artery.c_str());
    }
}

/**
 * @private
 *
 * read
 *
 * API call to the devices to ask for a reading
 * @return void
 */
void DeviceManager::read()
{
    waitForTrue(&DeviceManager::isNotPublishing, this, 10000);
    readBusy = true;
    iterateDevices(&DeviceManager::setReadCallback, this);
    read_count++;
    if (read_count >= MAX_SEND_TIME)
    {
        setReadCount(0);
    }

    readBusy = false;
    Utils::log("READ_EVENT", "READCOUNT=" + String(read_count));
}

/**
 * @private
 *
 * read
 *
 * Wrapper to setup devices for publishing
 * @return void
 */
void DeviceManager::publish()
{
    // checkBootThreshold();
    waitForTrue(&DeviceManager::isNotReading, this, 10000);
    Utils::log("PUBLICATION_EVENT", "EVENT=" + processor->getPublishTopic(false));
    // waitFor(DeviceManager::isNotReading, 10000);
    publishBusy = true;

    publisher();

    if (attempt_count < ATTEMPT_THRESHOLD)
    {
        attempt_count++;
    }
    else
    {
        Utils::log("ON_LINE_FALURE", StringFormat("OFFLINE COUNT %d", attempt_count));
        attempt_count = 0;
    }
    read_count = 0;
    publishBusy = false;
}

/**
 * @private
 *
 * getBufferSize
 *
 * Returns the buffer size for all connected devices
 * @return size_t
 */
// size_t DeviceManager::getBufferSize()
// {
//     size_t buff_size = DEFAULT_BUFFER_SIZE;
//     for (size_t i = 0; i < this->deviceCount; i++)
//     {
//         size_t size = this->deviceAggregateCounts[i];
//         for (size_t j = 0; j < size; j++)
//         {
//             size_t buff = this->devices[i][j]->buffSize();
//             if (buff)
//             {
//                 buff_size += buff;
//             }
//         }
//     }
//     if (!buff_size)
//     {
//         return DEFAULT_BUFFER_SIZE_MAX;
//     }

//     if (buff_size > DEFAULT_BUFFER_SIZE_MAX - 1)
//     {
//         return DEFAULT_BUFFER_SIZE_MAX;
//     }

//     return buff_size;
// }

/**
 * @private
 *
 * popOfflineCollection
 *
 * Asks the storage to pop off data stored while offline
 * @return void
 */
void DeviceManager::popOfflineCollection()
{
    // this->storage->popOfflineCollection();
}

/**
 * @private
 *
 * packagePayload
 *
 * Places the payload details in the header
 *
 * @param JSONBufferWriter *writer
 *
 * @return void
 */
void DeviceManager::packagePayload(JsonDocument &writer)
{
    writer["device"] = Hyphen.deviceID();
    writer["target"] = this->ROTATION;
    writer["date"] = Time.format(TIME_FORMAT_ISO8601_FULL);
}

/**
 * @private
 *
 * getTopic
 *
 * Returns the publication topic based on maintenance count
 *
 * @param uint8_t maintenanceCount
 *
 * @return bool
 */
bool DeviceManager::checkMaintenance(uint8_t maintenanceCount)
{
    bool recommended = recommendedMaintence(maintenanceCount);
    bool inMaintenance = boots.hasMaintenance();
    bool maintenance = recommended || inMaintenance;
    return maintenance;
}

/**
 * @private
 *
 * getTopic
 *
 * Returns the publication topic based on maintenance count
 *
 * @param uint8_t maintenanceCount
 *
 * @return String
 */
String DeviceManager::getTopic(bool maintenance)
{
    return processor->getPublishTopic(maintenance);
}

/**
 * @private
 *
 * payloadWriter
 *
 * Wraps the json buffer into a string
 *
 * @return String
 */
String DeviceManager::payloadWriter(uint8_t &maintenanceCount)
{

    // size_t bufferSize = getBufferSize();
    JsonDocument doc;
    packagePayload(doc);
    for (size_t i = 0; i < this->deviceCount; i++)
    {
        String name = "payload" + (i > 0 ? "-" + String(i) : "");
        JsonObject payload = doc[name].to<JsonObject>();
        // payload.begin();
        size_t size = this->deviceAggregateCounts[i];
        for (size_t j = 0; j < size; j++)
        {
            this->devices[i][j]->publish(payload, attempt_count);
            maintenanceCount += this->devices[i][j]->maintenanceCount();
        }
        // payload.end();
    }
    String output;
    serializeJson(doc, output);
    Serial.println(output);
    return output;
}

/**
 * @private
 *
 * publisher
 *
 * Gathers all data and sends to the processor the returned content
 * @return void
 */
void DeviceManager::publisher()
{
    attempt_count = 0;
    read_count = 0;
    uint8_t maintenanceCount = 0;
    String result = payloadWriter(maintenanceCount);
    bool maintenance = checkMaintenance(maintenanceCount);
    String topic = getTopic(maintenance);
    bool success = processor->publish(topic, result);

    Utils::log("SENDING_EVENT_READY " + topic, String(success));

    if (!maintenance && !success)
    {
        Utils::log("SENDING PAYLOAD FAILED. Storing", result);
        storePayload(result, topic);
    }

    else if (success)
    {
        popOfflineCollection();
    }
    clearArray();
    ROTATION++;
}

/**
 * @private
 *
 * buildSendInterval
 *
 * Called from a cloud function for setting up the timing mechanism
 * @return void
 */
void DeviceManager::buildSendInterval(int interval)
{
    setReadCount(0);
    clearArray();
    boots.buildSendInterval(interval);
}

/**
 * @private
 *
 * loopCallback
 *
 * Calls the device loop function during the iteration loop
 *
 * @return void
 *
 */
void DeviceManager::loopCallback(Device *device)
{
    device->loop();
}

/**
 * @private
 *
 * setParamsCountCallback
 *
 * Calls the device paramCount function during the iteration loop
 *
 * @return void
 *
 */
void DeviceManager::setParamsCountCallback(Device *device)
{
    uint8_t count = device->paramCount();
    this->paramsCount += count;
}

/**
 * @private
 *
 * restoreDefaultsCallback
 *
 * Calls the device restoreDefaults function during the iteration loop
 *
 * @return void
 *
 */
void DeviceManager::restoreDefaultsCallback(Device *device)
{
    device->restoreDefaults();
}

/**
 * @private
 *
 * initCallback
 *
 * Calls the device init function during the iteration loop
 *
 * @return void
 *
 */
void DeviceManager::initCallback(Device *device)
{
    device->init();
}

/**
 * @private
 *
 * clearArrayCallback
 *
 * Calls the device clear function during the iteration loop
 *
 * @return void
 *
 */
void DeviceManager::clearArrayCallback(Device *device)
{
    device->clear();
}

/**
 * @private
 *
 * setReadCallback
 *
 * Calls the device read function during the iteration loop
 *
 * @return void
 *
 */
void DeviceManager::setReadCallback(Device *device)
{
    device->read();
}

/**
 * @private
 *
 * isStrapped
 *
 * Checkts to see if bootstrap is finished bootstrapping
 *
 * @return void
 *
 */
bool DeviceManager::isStrapped()
{
    return this->boots.isStrapped();
}

/**
 * @private
 *
 * iterateDevices
 *
 * Iterates through all the devices and calls the supplied callback
 *
 * @return void
 *
 */
void DeviceManager::iterateDevices(void (DeviceManager::*iter)(Device *d), DeviceManager *binding)
{
    for (size_t i = 0; i < this->deviceCount; i++)
    {
        size_t size = this->deviceAggregateCounts[i];
        for (size_t j = 0; j < size; j++)
        {
            (binding->*iter)(this->devices[i][j]);
        }
    }
}

/**
 * @private
 *
 * waitForTrue
 *
 * Similie to particle's waitFor function but want it working with
 * member functions.
 *
 * @param bool() function - the function that needs to been called
 * @param unsigned long time - to wait for
 *
 * @return bool
 *
 */
bool DeviceManager::waitForTrue(bool (DeviceManager::*func)(), DeviceManager *binding, unsigned long time)
{
    bool valid = false;
    unsigned long then = millis();
    while (!valid && (millis() - time) < then)
    {
        valid = (binding->*func)();
    }
    return valid;
}

/**
 * @private
 *
 * setCloudFunctions
 *
 * sets the cloud functions from particle
 *
 * @return void
 *
 */
void DeviceManager::setCloudFunctions()
{
    Hyphen.function("setPublicationInterval", &DeviceManager::setSendInterval, this);
    Hyphen.function("restoreDefaults", &DeviceManager::restoreDefaults, this);
    Hyphen.function("reboot", &DeviceManager::rebootRequest, this);
    Hyphen.function("addDevice", &DeviceManager::addDevice, this);
    Hyphen.function("removeDevice", &DeviceManager::removeDevice, this);
    Hyphen.function("showDevices", &DeviceManager::showDevices, this);
    Hyphen.function("clearAllDevices", &DeviceManager::clearAllDevices, this);
}

/**
 * @private
 *
 * clearDeviceString
 *
 * Clears the device  string array
 *
 * @return size_t
 *
 */
void DeviceManager::clearDeviceString()
{
    for (uint8_t i = 0; i < MAX_DEVICES; i++)
    {
        devicesString[i] = "";
    }
}

/**
 * @private
 *
 * clearAllDevice
 *
 * Clears the device table and Persistance data
 *
 * @return size_t
 *
 */
int DeviceManager::clearAllDevices(String value)
{
    if (!value.equals("DELETE"))
    {
        return -1;
    }
    Utils::log("DEVICE_CLEARING_EVENT_WAS_CALLED", "clearing all data and values");
    boots.haultPublication();
    deviceAggregateCounts[ONE_I] = 0;
    clearDeviceString();
    clearArray();
    setReadCount(0);
    boots.clearDeviceConfigArray();
    boots.resumePublication();
    return 1;
}

/**
 * @private
 *
 * countDeviceType
 *
 * Counts the number of active devices with a current device tag
 *
 * @return size_t
 *
 */
size_t DeviceManager::countDeviceType(String deviceName)
{
    size_t count = 0;
    for (uint8_t i = 0; i < MAX_DEVICES; i++)
    {
        String device = devicesString[i];
        if (device.startsWith(deviceName))
        {
            count++;
        }
    }
    return count;
}

/**
 * @private
 *
 * violatesDeviceRules
 *
 * Checks the device string to see if it can process a request
 * to add the additional device
 *
 * @return bool
 *
 */
bool DeviceManager::violatesDeviceRules(String value)
{
    bool violation = true;
    // if it already contains the device.
    if (Utils::containsValue(devicesString, MAX_DEVICES, value) > -1)
    {
        return violation;
    }
    // get the config details
    String configurationStore[CONFIG_STORAGE_MAX];
    // put it into an array
    config.loadConfigurationStorage(value, configurationStore, CONFIG_STORAGE_MAX);
    String deviceName = configurationStore[DEVICE_NAME_INDEX];
    // if we have no device of this type, fail it
    int deviceIndex = config.getEnumIndex(deviceName);
    if (deviceIndex == -1)
    {
        return violation;
    }
    /**
     * We count the number and make sure there aren't too many
     */
    size_t occurrences = countDeviceType(deviceName);

    if (config.violatesOccurrences(deviceName, occurrences))
    {
        return violation;
    }

    return !violation;
}

/**
 * @private
 *
 * addDevice
 *
 * sets the cloud functions from particle
 *
 * @param String - value from the cloud function
 *
 * @return void
 *
 */
int DeviceManager::addDevice(String value)
{
    if (Utils::containsValue(devicesString, MAX_DEVICES, value) != -1)
    {
        return DEVICES_ALREADY_INSTANTIATED;
    }
    else if (this->deviceAggregateCounts[ONE_I] >= MAX_DEVICES)
    {
        return DEVICES_AT_MAX;
    }
    else if (violatesDeviceRules(value))
    {
        return DEVICES_VIOLATES_RULES;
    }

    int valid = applyDevice(config.addDevice(value, &boots), value, true);
    if (valid == 0)
    {
        return DEVICE_FAILED_TO_INSTANTIATE;
    }
    setParamsCount();
    boots.storeDevice(value, valid - 1);
    return valid;
}

/**
 * @private
 *
 * resetDeviceIndex
 *
 * Clears a device at a specific index
 *
 * @param int index
 *
 * @return void
 *
 */
void DeviceManager::resetDeviceIndex(size_t index)
{
    delete devices[ONE_I][index];
    devices[ONE_I][index] = new Device();
    devicesString[index] = "";
    boots.storeDevice("", index);
}

/**
 * @private
 *
 * copyDevicesFromIndex
 *
 * Moves the device list from the current index to next index
 *
 * @param String - value from the cloud function
 *
 * @return int
 *
 */
void DeviceManager::copyDevicesFromIndex(int index)
{
    // we are at the last index
    if ((size_t)index >= deviceAggregateCounts[ONE_I] - 1)
    {
        return resetDeviceIndex(index);
    }

    for (size_t i = index + 1; i < deviceAggregateCounts[ONE_I]; i++)
    {
        // kill this object
        delete devices[ONE_I][i - 1];
        // set the one prior
        devices[ONE_I][i - 1] = devices[ONE_I][i];
        // and kill this one too.
        // delete devices[ONE_I][i];
        devices[ONE_I][i] = new Device();
        String name = devicesString[i];
        if (!name.equals(""))
        {
            devicesString[i - 1] = name;
            boots.storeDevice(name, i - 1);
        }
        devicesString[i] = "";
        // we are going to remove this reference
        boots.storeDevice("", i);
    }
}

/**
 * @private
 *
 * setCloudFunctions
 *
 * sets the cloud functions from particle
 *
 * @param String - value from the cloud function
 *
 * @return int
 *
 */
int DeviceManager::removeDevice(String value)
{
    Utils::log("DEVICE_REMOVAL_EVENT_CALLED", value);
    int valid = Utils::containsValue(devicesString, MAX_DEVICES, value);
    if (valid > -1)
    {
        boots.haultPublication();
        setReadCount(0);
        copyDevicesFromIndex(valid);
        deviceAggregateCounts[ONE_I]--;
        clearArray();
        boots.resumePublication();
    }
    return valid;
}

/**
 * @private
 *
 * publishDeviceList
 *
 * sends a list of devices via the processor
 *
 * @return bool
 *
 */
bool DeviceManager::publishDeviceList()
{
    // JSONBufferWriter writer = createJSONBuffer(BUFF_SIZE);
    // char buf[BUFF_SIZE];
    // memset(buf, 0, sizeof(buf));
    JsonDocument doc;

    packagePayload(doc);
    JsonObject payload = doc["payload"].to<JsonObject>();
    ;
    for (size_t i = 0; i < MAX_DEVICES; i++)
    {
        String d = devicesString[i];
        payload[String(i)] = d;
    }

    String output;
    serializeJson(doc, output);
    return processor->publish(AI_DEVICE_LIST_EVENT, output.c_str());
}

/**
 * @private
 *
 * showDevices
 *
 * Pulls the devices and sends the payload over the processor
 * @param String - value from the cloud function
 *
 * @return int
 *
 */
int DeviceManager::showDevices(String value)
{
    int valid = publishDeviceList() ? 1 : 0;
    return valid;
}

/**
 * @private
 *
 * strapDevices
 *
 * Pulls the devices from bootstraps EPROM
 *
 * @return void
 */
void DeviceManager::strapDevices()
{
    boots.strapDevices(devicesString);
    for (uint8_t i = 0; i < MAX_DEVICES; i++)
    {
        String device = devicesString[i];
        Utils::log("BOOTSTRAPPING_DEVICE_________________________", device);
        if (!device.equals(""))
        {
            applyDevice(config.addDevice(device, &boots), device, true);
        }
        Utils::log("_____________________________________________", "\n");
    }
}

/**
 * @private
 *
 * applyDevice
 *
 * Adds the device to the scope
 *
 * @return void
 *
 */
int DeviceManager::applyDevice(Device *device, String deviceString, bool startup)
{
    // we also need to make sure it's cool with the rules
    if (device == NULL)
    {
        return Utils::log("BOOTSTRAPPING_DEVICE_FAILED", deviceString, 0);
    }

    boots.haultPublication();
    // we should clear the devices since we are resetting the clocks
    // We are only working with the first dimension
    this->devices[ONE_I][deviceAggregateCounts[ONE_I]] = device;
    // Run the init function
    this->devices[ONE_I][deviceAggregateCounts[ONE_I]]->init();
    // Set the device name
    this->devicesString[deviceAggregateCounts[ONE_I]] = deviceString;
    // Increment Count
    this->deviceAggregateCounts[ONE_I]++;
    if (!startup)
    {
        this->clearArray();
    }
    setReadCount(0);
    boots.resumePublication();
    return this->deviceAggregateCounts[ONE_I];
}
