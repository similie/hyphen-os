#include "configurator.h"

/**
 * Deconstructor
 *
 * */
Configurator::~Configurator()
{
}
/**
 * Constructor
 *
 * */
Configurator::Configurator()
{
}

/**
 * @public
 *
 * noIdentity
 *
 * Checks to see if the payload comes without an identity of if that is
 * required
 *
 * @param String configurationStore
 * @param int index - the enum index
 *
 * @return Device *
 *
 */
bool Configurator::noIdentity(String configurationStore[], int index)
{
    String identity = configurationStore[DEVICE_IDENTITY_INDEX];
    bool noIdentity = identity.equals("");
    // these are device that don't need a config param
    if (noIdentity && (index == rain_gauge || index == gps_device || index == battery))
    {
        noIdentity = false;
    }
    else
    {
        int number = parseIdentity(identity);
        noIdentity = isnan(number);
    }
    return noIdentity;
}

/**
 * @public
 *
 * violagesOccurances
 *
 * We only want a certain number of specific devices attached
 *
 * @param String value
 * @param int occurances
 *
 * @return bool - does it violate
 *
 *
 */
bool Configurator::violatesOccurrences(String value, int occurs)
{
    int index = getEnumIndex(value);
    switch (index)
    {
    case all_weather:
        return occurs > 1;
    case soil_moisture:
        return occurs > 2;
    case rain_gauge:
        return occurs > 1;
    case gps_device:
        return occurs > 1;
    case battery:
        return occurs > 1;
    case sonic_sensor:
        return false;
    case beco_flow_meter:
        return occurs > 2;
    case flow_meter:
        return false;
    case video_capture:
        return occurs > 1;
    case accelerometer:
        return occurs > 1;
    case ip_camera:
        // it would be cool to have more than one
        // return occurs > 1;
        return false;
    default:
        return false;
    }
}

/**
 * @public
 *
 * loadConfigurationStorage
 *
 * Pulls device from the String representation
 *
 * @param String payload
 * @param String configurationStore
 *
 * @return Device *
 *
 *
 */
void Configurator::loadConfigurationStorage(String payload, String configurationStore[], size_t size)
{
    size_t j = 0;
    uint8_t i = 0;
    String value = "";
    while (j < payload.length() && i < size)
    {
        char c = payload.charAt(j);
        if (c == ':')
        {
            configurationStore[i] = value;
            value = "";
            i++;
        }
        else
        {
            value += String(c);
        }

        j++;
    }
    if (i < size)
    {
        configurationStore[i] = value;
    }
    Utils::log("CONFIGURATION_DEFINITION FOR ", payload);
}

/**
 * @public
 *
 * getEnumIndex
 *
 * Gets the index of the device enum
 *
 * @param String value
 *
 * @return int - index of the enum
 *
 */
int Configurator::getEnumIndex(String value)
{
    return Utils::containsValue(devicesAvailable, CURRENT_DEVICES_COUNT, value);
}

/**
 * @public
 *
 * addDevice
 *
 * Gets the device from configuration string
 *
 * @param String payload
 * @param Bootstrap * boots
 *
 * @return Device *
 *
 */
Device *Configurator::addDevice(String payload, Bootstrap *boots)
{
    Utils::log("ADDING_DEVICE", payload);
    String configurationStore[CONFIG_STORAGE_MAX];
    loadConfigurationStorage(payload, configurationStore, CONFIG_STORAGE_MAX);
    return pullDeviceType(configurationStore, boots);
}

/**
 * @private
 *
 * parseIdentity
 *
 * Gets the integer value from the string
 *
 * @param String payload
 *
 * @return int
 *
 */
int Configurator::parseIdentity(String value)
{
    return atoi(value.c_str());
}

/**
 * @private
 *
 * pullDeviceType
 *
 * Pulls device from the String representation by iterating the device enum
 *
 * @param String configurationStore[]
 * @param Bootstrap * boots
 *
 * @return Device *
 *
 */
Device *Configurator::pullDeviceType(String configurationStore[], Bootstrap *boots)
{
    int index = getEnumIndex(configurationStore[DEVICE_NAME_INDEX]);

    if (index == -1 || noIdentity(configurationStore, index))
    {
        return NULL;
    }

    switch (index)
    {
    case all_weather:
        return new AllWeather(boots,
                              parseIdentity(configurationStore[DEVICE_IDENTITY_INDEX]));
    case soil_moisture:
        return new SoilMoisture(boots,
                                parseIdentity(configurationStore[DEVICE_IDENTITY_INDEX]));
    case rain_gauge:
        return new RainGauge(boots);
    case gps_device:
        return new GpsDevice();
    case battery:
        return new Battery();
    case sonic_sensor:
        if (!configurationStore[DEVICE_PIN_INDEX].equals(""))
        {
            return new WlDevice(boots,
                                parseIdentity(configurationStore[DEVICE_IDENTITY_INDEX]),
                                parseIdentity(configurationStore[DEVICE_PIN_INDEX]));
        }
        else if (configurationStore[DEVICE_PIN_INDEX].equals("") && !configurationStore[DEVICE_IDENTITY_INDEX].equals(""))
        {
            return new WlDevice(boots, 0,
                                parseIdentity(configurationStore[DEVICE_IDENTITY_INDEX]));
        }
        else if (!configurationStore[DEVICE_IDENTITY_INDEX].equals(""))
        {
            return new WlDevice(boots,
                                parseIdentity(configurationStore[DEVICE_IDENTITY_INDEX]));
        }
        return new WlDevice(boots);
    case flow_meter:
        if (!configurationStore[DEVICE_PIN_INDEX].equals(""))
        {
            return new FlowMeter(boots,
                                 parseIdentity(configurationStore[DEVICE_IDENTITY_INDEX]),
                                 parseIdentity(configurationStore[DEVICE_PIN_INDEX]));
        }
        else if (configurationStore[DEVICE_IDENTITY_INDEX].equals(""))
        {
            return new FlowMeter(boots,
                                 parseIdentity(configurationStore[DEVICE_IDENTITY_INDEX]));
        }
        return new FlowMeter(boots);
    case video_capture:
        return new VideoCapture(boots);
    case relay:
        if (!configurationStore[DEVICE_PIN_INDEX].equals(""))
        {
            return new Relay(boots, parseIdentity(configurationStore[DEVICE_PIN_INDEX]), configurationStore[DEVICE_IDENTITY_INDEX]);
        }
        else if (!configurationStore[DEVICE_IDENTITY_INDEX].equals(""))
        {
            return new Relay(boots, parseIdentity(configurationStore[DEVICE_IDENTITY_INDEX]));
        }
        return new Relay(boots);
    case accelerometer:
        return new Accelerometer(boots);
    case ip_camera:
        return new IPCamera(boots);
    default:
        return NULL;
    }
}
