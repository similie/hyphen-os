#include "utils.h"

static bool debugValue = false;

/**
 * @deconstructor
 */
Utils::~Utils()
{
}

/**
 * @constructor
 */
Utils::Utils()
{
}

/**
 * @public
 *
 * parseCloudFunctionInteger
 *
 * Parses a string into a double. Used for parse cloud functions
 * that are a string into the double value
 *
 * @param String value - the cloud value
 * @param String name - the name of the device requesting
 *
 * @return int
 *
 */
int Utils::parseCloudFunctionInteger(String value, String name)
{
    const char *stringCal = value.c_str();
    double val = ::atoi(stringCal);
    Utils::log("SETTING_CALIBRATION_" + name, "value: " + String(stringCal));
    return val;
}

/**
 * @public
 *
 * parseCloudFunctionDouble
 *
 * Parses a string into a double. Used for parse cloud functions
 * that are a string into the double value
 *
 * @param String value - the cloud value
 * @param String name - the name of the device requesting
 *
 * @return double
 *
 */
double Utils::parseCloudFunctionDouble(String value, String name)
{
    const char *stringCal = value.c_str();
    double val = ::atof(stringCal);
    Utils::log("SETTING_CALIBRATION_" + name, "value: " + String(stringCal));
    return val;
}

/**
 * @public
 *
 * requestDeviceId
 *
 * Returns a concatenated String that contains the request identity, and
 * command intended over the serial bus
 *
 * @param int identity - devices specific identity
 * @param String name - the command being sent over the serial bus
 *
 * @return String
 *
 */
String Utils::requestDeviceId(int identity, String cmd)
{
    return "request_" + String(identity) + " " + cmd;
}

/**
 * @public
 *
 * receiveDeviceId
 *
 * Applies the request identity for pulled data off a serialbus
 * for devices with assinged an identity.
 *
 * @param int identity - devices specific identity
 *
 * @return String
 *
 */
String Utils::receiveDeviceId(int identity)
{
    return "result_" + String(identity);
}

/**
 * @public
 *
 * serialMessageHasError
 *
 * Checks to see if there is an error on the serial bus for a specific device
 *
 * @param String message - the message off the serial bus
 * @param int identity - devices specific identity
 *
 * @return String
 *
 */
bool Utils::serialMessageHasError(String message, int identity)
{
    bool error = false;
    if (message.startsWith("ERROR_" + String(identity)))
    {
        message.replace("ERROR_" + String(identity), "");
        String errorMessage = " FOR " + String(identity) + " WITH MESSAGE:: " + message;
        this->log("SERIAL_BUS_ERROR", errorMessage);
        error = true;
    }
    return error;
}

/**
 * @public
 *
 * hasSerialIdentity
 *
 * Checks an ID to see if has been registered with a value greater than -1 (default)
 *
 * @param int identity - devices specific identity
 *
 * @return bool
 *
 */
bool Utils::hasSerialIdentity(int identity)
{
    return identity > -1;
}

/**
 * @public
 *
 * inValidMessageString
 *
 * Checks to see if a serial payload is valid for storage
 *
 * @param String message - the message off the serial bus
 * @param int identity - devices specific identity
 *
 * @return bool
 *
 */
bool Utils::inValidMessageString(String message, int identity)
{
    return serialMessageHasError(message, identity) ||
           (hasSerialIdentity(identity) &&
            !message.startsWith(receiveDeviceId(identity)));
}

/**
 * @brief Get the Converted Address Cmd object
 *
 * @param String cmd
 * @param String identity
 * @return String - the converted string
 */
String Utils::getConvertedAddressCmd(String cmd, int identity)
{
    int localIdentity = 0;
    if (hasSerialIdentity(identity))
    {
        localIdentity = identity;
    }
    cmd.replace("~", String(localIdentity));
    return cmd;
}

/**
 * @public
 *
 * getTimePadding
 *
 * Prefixes milliseconds with zero values for 10 bytes. Used in logging
 *
 * @return String - the milliseconds since boot padded with zeros
 *
 */
String getTimePadding()
{
    String time = String(millis());
    uint8_t pad = 10 - time.length();
    String padding = "";
    for (uint8_t i = 0; i < pad; i++)
    {
        padding += "0";
    }

    return padding + time;
}

/**
 * @public
 *
 * validConfigIdentity
 *
 * All stored config should have a param version set to 1
 *
 * @return uint8_t identity - the version value of the config
 *
 */
bool Utils::validConfigIdentity(uint8_t identity)
{
    return identity == 1;
}

/**
 * @public
 *
 * log
 *
 * Logs messages to the console
 *
 * @todo Use a template variant to accept parameters other
 * than a String value
 *
 * @param String event - the context for the logging
 * @param String message - the actual message
 *
 * @return void
 *
 */
void Utils::log(String event, String message)
{
    /**
     * For devices we deploy, we can shutdown this logging attribute
     * Value is found under bootstrap.h
     */
    if (!debugValue)
    {
        return;
    }
    String log = getTimePadding() + " [SIMILIE] " + Utils::removeNewLine(event) + ": " + Utils::removeNewLine(message);
    Serial.println(log);
    Utils::logToFile(log);
    Blue.log(log, LoggingDetails::LOGGING);
}

void Utils::logToFile(String log)
{
    if (!LOG_TO_FILE)
    {
        return;
    }

    // Storage.appendlnAsync(String(LOG_FILE_NAME), log, MAX_LOG_SIZE);

    uint64_t size = Storage.appendln(String(LOG_FILE_NAME), log);
    if (size > MAX_LOG_SIZE)
    {
        Storage.overwrite(LOG_FILE_NAME, "");
    }
}

/**
 * @public
 *
 * log
 *
 * Wrapper to log with error code
 *
 * @todo Use a template variant to accept parameters other
 * than a String value
 *
 * @param String event - the context for the logging
 * @param String message - the actual message
 * @param int errorCode - any code you want to slip in
 *
 * @return int
 *
 */
int Utils::log(String event, String message, int errorCode)
{
    log(event, message);
    return errorCode;
}

/**
 * @private public
 *
 * machineName
 *
 * converts a string to an long for searachble
 * EEPROM storage
 * @param String name - the device name
 * @return unsigned long
 */
unsigned long Utils::machineName(String name, bool unique)
{
    unsigned long manchineName = 0;
    uint16_t multiple = unique ? name.length() : 1;
    for (uint8_t i = 0; i < name.length(); i++)
    {
        char c = name.charAt(i);
        manchineName += (unsigned long)c * multiple;
    }
    return manchineName;
}

/**
 * @private public
 *
 * machineNameDirect
 *
 * converts a string to an long for searachble
 * EEPROM storage
 * @param String name - the device name
 * @param byte * restore
 *
 * @return unsigned long
 */
void Utils::machineNameDirect(String name, byte *save)
{
    name.getBytes(save, name.length() + 1);
}

/**
 * @private public
 *
 * machineToReadableName
 *
 * Reverts manchine name values back to a string
 *
 * @param byte * restore
 * @return String
 */
String Utils::machineToReadableName(byte *restore)
{
    String send = "";

    byte b = 255;
    uint8_t i = 0;
    while (b != 0 && i < DEVICE_BYTE_BUFFER_SIZE)
    {
        b = restore[i];

        char c = (char)b;
        send += String(c);
        i++;
    }

    return send;
}

/**
 * @public
 *
 * containsValue
 *
 * Does the array contain a specifig string value
 *
 * @param String arr[] - the array
 * @param size_t size - array size
 * @param String value - the value to search
 *
 * @return bool
 *
 */
int Utils::containsValue(String arr[], size_t size, String value)
{
    int has = -1;
    for (size_t i = 0; i < size; i++)
    {
        String check = arr[i];
        if (check.equals(value))
        {
            has = i;
            break;
        }
    }
    return has;
}

/**
 * @brief
 *
 * Gets the index of a string in an array
 *
 * @param key
 * @param arr
 * @param paramLength
 * @return int
 */

int Utils::getIndexOf(String key, String arr[], size_t paramLength)
{
    for (size_t i = 0; i < paramLength; i++)
    {
        if (arr[i].equals(key))
        {
            return i;
        }
    }
    return -1;
}

void Utils::fillParseSplitReadSerial(String ourReading, size_t paramLength, size_t max, String nameMap[], float value_hold[][Constants::OVERFLOW_VAL])
{
    size_t j = 1;
    String param = "";
    for (size_t i = 0; i < ourReading.length(); i++)
    {
        j = i;
        char c = ourReading.charAt(i);
        if (c == ',')
        {
            continue;
        }
        else if (c == '=')
        {
            j++;
            String buff = "";
            char d = ourReading.charAt(j);
            while (d != ',' && d != '\n' && d != '\0')
            {
                buff += String(d);
                j++;
                d = ourReading.charAt(j);
            }

            if (invalidNumber(buff))
            {
                buff = "9999";
            }
            i = j;
            float value = buff.toInt();
            int index = Utils::getIndexOf(param, nameMap, paramLength);
            param = "";
            if (index > -1)
            {
                insertValue(value, value_hold[index], max);
            }
        }
        else
        {
            param += String(c);
        }
    }
}

/**
 * @brief
 *
 * Used when parsing the serial payload
 *
 * @param ourReading
 * @param paramLength
 * @param max
 * @param nameMap
 * @param value_hold
 */
void Utils::parseSplitReadSerial(String ourReading, size_t paramLength, size_t max, String nameMap[], float value_hold[][Constants::OVERFLOW_VAL])
{
    size_t j = 1;
    String param = "";
    for (size_t i = 0; i < ourReading.length(); i++)
    {
        j = i;
        char c = ourReading.charAt(i);
        if (c == ',')
        {
            continue;
        }
        else if (c == '=')
        {
            j++;
            String buff = "";
            char d = ourReading.charAt(j);
            while (d != ',' && d != '\n' && d != '\0')
            {
                buff += String(d);
                j++;
                d = ourReading.charAt(j);
            }

            if (this->invalidNumber(buff))
            {
                buff = "9999";
            }
            i = j;
            float value = buff.toInt();
            int index = Utils::getIndexOf(param, nameMap, paramLength);
            param = "";
            if (index > -1)
            {
                this->insertValue(value, value_hold[index], max);
            }
        }
        else
        {
            param += String(c);
        }
    }
}

/**
 * @brief
 *
 * Strips the ID from the payload
 *
 * @param String ourReading
 * @return String
 */
String Utils::removeSensorIdFromPayload(String ourReading)
{
    String value = "";
    boolean progress = false;
    for (size_t i = 0; i < ourReading.length(); i++)
    {
        char c = ourReading.charAt(i);
        if ((c == '+' || c == '-') && !progress)
        {
            progress = true;
            continue;
        }
        else if (!progress)
        {
            continue;
        }
        value += String(c);
    }
    return value;
}

/**
 * @brief
 *
 * checks the character to determine if it is a valid non-stop character
 *
 * @param char d
 * @return true if this isn't a control character
 */
bool Utils::notStopCheckChar(char d)
{
    return d != '+' && d != '-' && d != '\n' && d != '\0' && d != 13;
}

/**
 * @brief
 *
 * parses the string from the SDI-12 interface sensors
 *
 * @param String ourReading our string for parsing
 * @param String * values - the array to fill
 * @param size_t maxLength  the max length of the array
 */
void Utils::splitStringToValues(String ourReading, String *values, size_t maxLength)
{
    String cleanedReading = removeSensorIdFromPayload(ourReading);
    size_t length = cleanedReading.length();
    size_t index = 0;
    size_t storageIndex = 0;
    while (index < length && storageIndex < maxLength)
    {
        char c = cleanedReading.charAt(index);
        if (!notStopCheckChar(c))
        {
            index++;
            continue;
        }
        char scale = cleanedReading.charAt(index - 1);
        String startChar = scale == '+' ? "" : String(scale);
        String build = startChar;
        size_t valueLength = 0;
        while (notStopCheckChar(c))
        {
            build += String(c);
            valueLength++;
            c = cleanedReading.charAt(index + valueLength);
        }
        index += valueLength;
        values[storageIndex] = build.equals("-") || build.equals("") ? String(FAILED_VALUE) : build;
        storageIndex++;
    }
}

void Utils::fillStringifiedFailedDefaults(String *values, size_t length)
{
    for (size_t i = 0; i < length; i++)
    {
        values[i] = String(FAILED_VALUE);
    }
}

/**
 * @public
 *
 * parseSerial
 *
 * Parses serial strings sent over the serial bus for Meter SDI-12 devices
 *
 * @param String ourReading - the serial details
 * @param size_t paramLength - the number of parameters to parse
 * @param size_t max - the max value for the valuehold array
 * @param float value_hold[][Constants::OVERFLOW_VAL] - a multi-dimentional array holding the values
 *
 * @return void
 *
 */
void Utils::parseSerial(String ourReading, size_t paramLength, size_t max, float value_hold[][Constants::OVERFLOW_VAL])
{
    String stringifiedValues[paramLength];
    fillStringifiedFailedDefaults(stringifiedValues, paramLength);
    splitStringToValues(ourReading, stringifiedValues, paramLength);
    for (size_t i = 0; i < paramLength; i++)
    {
        String value = stringifiedValues[i];
        if (this->invalidNumber(value))
        {
            continue;
        }
        float storedValue = NO_VALUE;
        if (this->containsChar('.', value))
        {
            storedValue = value.toFloat();
        }
        else
        {
            storedValue = value.toInt();
        }

        if (isnan(storedValue))
        {
            continue;
        }
        insertValue(storedValue, value_hold[i], max);
    }
}

/**
 * @public
 *
 * skipMultiple
 *
 * Some devices don't want or cant process a read on every interval. This sets values
 * where a read is acceptable
 *
 * @param unsigned int size - the size requested from a 15 interval
 * @param size_t maxVal - the max interval, normally 15
 * @param unsigned int threshold - max to do in a given interval
 *
 * @return size_t
 *
 */
size_t Utils::skipMultiple(unsigned int size, size_t maxVal, unsigned int threshold)
{
    size_t start = 1;
    for (size_t i = 1; i < maxVal; i++)
    {
        unsigned int test = size * i;
        float multiple = (float)test / threshold;
        if (multiple >= 1)
        {
            start = i;
            break;
        }
    }
    return start;
}

/**
 * @public
 *
 * simCallback
 *
 * Callback function used during beach mode to talk to the sym.
 *
 * @todo add functionality
 *
 * @param int type
 * @param const char *buf
 * @param int len
 * @param char *value
 *
 * @return size_t
 *
 */
int Utils::simCallback(int type, const char *buf, int len, char *value)
{
#if PLATFORM_ID == 13
    if ((type == TYPE_PLUS) && value)
    {
        // @todo
    }
    return WAIT;
#else
    return 0;
#endif
}

/**
 * @public
 *
 * connected
 *
 * Wraps system functions to check of the system is online
 *
 *
 * @return bool
 *
 */
bool Utils::connected()
{
#if PLATFORM_ID == 13
    return Particle.connected() || Cellular.ready();
#else
    return Hyphen.connected();
#endif
}

/**
 * @public
 *
 * getSum
 *
 * Sums all values in a float array
 *
 * @param float values[] - the values to sum
 * @param size_t MAX - the size of the array to scan
 *
 * @return float
 *
 */
float Utils::getSum(float values[], size_t MAX)
{
    int sum = 0;
    const int THRESHOLD_VAL = 100;
    const int DIVISOR = 100;
    const int THRESHOLD = DIVISOR * THRESHOLD_VAL;
    bool hasValue = false;
    size_t MIN = 0;
    size_t last = 0;
    for (size_t i = MIN; i < MAX; i++)
    {
        if (values[i] == NO_VALUE)
        {
            continue;
        }
        int val = (int)((float)values[i] * DIVISOR);
        // this might offer a buffer
        int delta = val - last;
        if (abs(delta) > THRESHOLD)
        {
            continue;
        }
        last = val;
        hasValue = true;
        sum += val;
    }

    if (!hasValue)
    {
        return NO_VALUE;
    }

    return (float)sum / DIVISOR;
}

/**
 * @public
 *
 * getMax
 *
 * Gets the max value of an array
 *
 * @param float values[] - the values to sum
 * @param size_t MAX - the size of the array to scan
 *
 * @return float
 *
 */
float Utils::getMax(float values[], size_t MAX)
{
    float value = 0;
    size_t MIN = 0;
    size_t i = MAX;
    while (i > MIN)
    {

        value = values[i - 1];
        if (value != NO_VALUE)
        {
            return value;
        }
        i--;
    }
    return value;
}

/**
 * @brief checks to see if there is a match against string values
 *
 * @param value
 * @param match
 * @return true
 * @return false
 */
bool Utils::containsString(String value, String match)
{
    return value.indexOf(match) >= 0;
}

/**
 * @public
 *
 * getMedian
 *
 * Get the middle value or the value toward the zero index
 * this is not a NO_VALUE
 *
 * @param float values[] - the values to sum
 * @param size_t MAX - the size of the array to scan
 *
 * @return float
 *
 */
float Utils::getMedian(float arr[], size_t max)
{
    float center = (float)max / 2.0;
    int index = ceil(center);
    float value = NO_VALUE;
    while (value == NO_VALUE && index >= 0)
    {
        value = arr[index];
        index--;
    }
    return value;
}

/**
 * @public
 *
 * containsChar
 *
 * Does the String contain a specific character
 *
 * @param char c - the character
 * @param String readFrom - the read string
 *
 * @return bool
 *
 */
bool Utils::containsChar(char c, String readFrom)
{
    bool contains = false;
    for (u_int i = 0; i < readFrom.length(); i++)
    {
        char check = readFrom.charAt(i);
        if (check == c)
        {
            contains = true;
            break;
        }
    }
    return contains;
}

/**
 * @public
 *
 * invalidNumber
 *
 * Is the string an invalid number representation
 *
 * @param String value - the read string
 *
 * @return bool
 *
 */
bool Utils::invalidNumber(String value)
{
    bool invalid = false;
    for (uint16_t i = 0; i < value.length(); i++)
    {
        char c = value.charAt(i);
        // we leave in the / char for simplicity
        if (c >= '-' && c <= '9')
        {
            continue;
        }
        invalid = true;
        break;
    }
    return invalid;
}

/**
 * @public
 *
 * shift
 *
 * Helper fuction that inserts an element in asscending order into
 * the sorted array where we can our median value
 * @param float value - the value to be inserted
 * @param size_t index - the index where to insert;
 * @param float arr[] - the array to insert into
 * @param size_t size - size of the array
 *
 * @return void;
 */
void Utils::shift(float value, size_t index, float arr[], size_t size)
{

    float last = arr[index];
    arr[index] = value;
    index++;

    while (index < size)
    {
        float temp = arr[index];
        arr[index] = last;
        index++;
        if (index < size)
        {
            last = arr[index];
            arr[index] = temp;
            index++;
        }
    }
}

/**
 * @public
 *
 * shift
 *
 * Helper fuction that inserts an element in asscending order into
 * the sorted array where we can our median value
 * @param int value - the value to be inserted
 * @param size_t index - the index where to insert;
 * @param int arr[] - the array to insert into
 * @param size_t size - size of the array
 *
 * @return void;
 */
void Utils::shift(int value, size_t index, int arr[], size_t size)
{

    int last = arr[index];
    arr[index] = value;
    index++;

    while (index < size)
    {
        int temp = arr[index];
        arr[index] = last;
        index++;
        if (index < size)
        {
            last = arr[index];
            arr[index] = temp;
            index++;
        }
    }
}

/**
 * @public
 *
 * shift
 *
 * Helper fuction that inserts an element in asscending order into
 * the sorted array where we can our median value
 * @param long value - the value to be inserted
 * @param size_t index - the index where to insert;
 * @param long arr[] - the array to insert into
 * @param size_t size - size of the array
 *
 * @return void;
 */
void Utils::shift(long value, size_t index, long arr[], size_t size)
{

    long last = arr[index];
    arr[index] = value;
    index++;

    while (index < size)
    {
        long temp = arr[index];
        arr[index] = last;
        index++;
        if (index < size)
        {
            last = arr[index];
            arr[index] = temp;
            index++;
        }
    }
}

/**
 * @public
 *
 * insertValue
 *
 * Insert a value into a multidimentional array in sorted order
 *
 * @param float value - the value to be inserted
 * @param int param - the enum value that represents the param
 * @param size_t size - the size of the array
 *
 * @return void;
 */
void Utils::insertValue(float value, float arr[], size_t size)
{
    size_t index = 0;
    float aggr = arr[index];
    while (value >= aggr && aggr != NO_VALUE && index < size)
    {
        index++;
        aggr = arr[index];
    }
    shift(value, index, arr, size);
}

/**
 * @public
 *
 * insertValue
 *
 * Insert a value into a multidimentional array in sorted order
 *
 *
 * @param int value - the value to be inserted
 * @param int param - the enum value that represents the param
 * @param size_t size - the size of the array
 *
 * @return void;
 */
void Utils::insertValue(long value, long arr[], size_t size)
{
    size_t index = 0;

    long aggr = arr[index];

    while (value >= aggr && aggr != NO_VALUE && index < size)
    {
        index++;
        aggr = arr[index];
    }
    shift(value, index, arr, size);
}

/**
 * @public
 *
 * insertValue
 *
 * Insert a value into a multidimentional array in sorted order
 *
 *
 * @param int value - the value to be inserted
 * @param int param - the enum value that represents the param
 * @param size_t size - the size of the array
 *
 * @return void;
 */
void Utils::insertValue(int value, int arr[], size_t size)
{
    size_t index = 0;

    int aggr = arr[index];

    while (value >= aggr && aggr != NO_VALUE && index < size)
    {
        index++;
        aggr = arr[index];
    }
    shift(value, index, arr, size);
}

/**
 * @public
 *
 * getMedian
 *
 * Get the middle value or the value toward the zero index
 * this is not a NO_VALUE
 *
 * @param long readparam - the expected max value
 * @param long arr[] - the array of values
 *
 * @return long
 *
 */
long Utils::getMedian(long readparam, long arr[])
{
    double center = (double)readparam / 2.0;
    int index = ceil(center);
    long value = NO_VALUE;
    while (value == NO_VALUE && index >= 0)
    {
        value = arr[index];
        index--;
    }
    return value;
}

/**
 * @public
 *
 * getMedian
 *
 * Get the middle value or the value toward the zero index
 * this is not a NO_VALUE
 *
 * @param int readparam - the expected max value
 * @param int arr[] - the array of values
 *
 * @return int
 *
 */
int Utils::getMedian(int readparam, int arr[])
{
    float center = (float)readparam / 2.0;
    int index = ceil(center);
    int value = NO_VALUE;
    while (value == NO_VALUE && index >= 0)
    {
        value = arr[index];
        index--;
    }
    return value;
}

/**
 * @public
 *
 * stringConvert
 *
 * converts string to a char *
 *
 * @param: String value - to be converted
 *
 * @return const * char
 *
 */
const char *Utils::stringConvert(String value)
{
    return value.c_str();
}

/**
 * @public
 *
 * reboot
 *
 * Wrapper to reboot the system
 *
 * @return void
 *
 */
void Utils::reboot()
{
    Hyphen.reset();
}

/**
 * @public
 *
 * setDebug
 *
 * turns on and off system logging
 *
 * @return void
 *
 */
void Utils::setDebug(bool debug)
{
    debugValue = debug;
}

/**
 * @brief removes the \n character from the end of a string
 *
 */
String Utils::removeNewLine(String value)
{
    if (!value.endsWith("\n"))
    {
        return value;
    }
    return value.substring(0, value.length() - 1);
}
