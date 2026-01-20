#define SDI12_BUFFER_SIZE 128

#include <SDI12.h>

#include "string.h"
#include "resources/devices/device.h"
#include "math.h"
#include "resources/bootstrap/bootstrap.h"
#include "resources/processors/LocalProcessor.h"
#include "resources/utils/utils.h"
#include <stdint.h>

#define SINGLE_SAMPLE true
#define READ_ON_LOW_ONLY false
#define DEVICE_CONNECTED_PIN 13
#define SDI12_PIN 15
#define SDI12_WAIT_READ 300
#ifndef sdi_object
#define sdi_object

/**
 * @brief We do this because we want to have a single instance of the SDI12 object
 * There are a number of devices using this object and we want to ensure that we only have one.
 * throughout the application lifecycle.
 */
class SDI12DeviceManager
{
private:
    SDI12 sdi12;
    SDI12DeviceManager() : sdi12(SDI12_PIN)
    {
    }

    ~SDI12DeviceManager()
    {
        sdi12.end();
    }

public:
    static SDI12DeviceManager &getInstance()
    {
        static SDI12DeviceManager instance;
        return instance;
    }

    void sendCommand(String cmd)
    {
        return sdi12.sendCommand(cmd);
    }

    int read()
    {
        return sdi12.read();
    }

    int available()
    {
        return sdi12.available();
    }

    void start()
    {
        if (sdi12.isActive())
        {
            return;
        }

        // sdi12.setTimeoutValue(1000);
        sdi12.begin();
        delay(1000);
    }

    SDI12DeviceManager(const SDI12DeviceManager &) = delete;
    SDI12DeviceManager &operator=(const SDI12DeviceManager &) = delete;
};

#endif

#ifndef sdi_12_h
#define sdi_12_h

enum
{
    impossible_index = 999
};

class SDIParamElements
{
private:
    String values[0] = {};

protected:
    Utils utils;

public:
    float valueHold[Constants::OVERFLOW_VAL][Constants::OVERFLOW_VAL];
    virtual String getDeviceName()
    {
        return "SDIDevice";
    }
    virtual size_t getBuffSize()
    {
        return 0;
    }
    virtual uint8_t getTotalSize()
    {
        return 0;
    }
    virtual String *getValueMap()
    {
        return values;
    }
    virtual float extractValue(float values[], size_t key, size_t max)
    {
        return 0;
    }
    float *getMappedValue(uint8_t iteration)
    {
        return valueHold[iteration];
    }
    float getMappedValue(uint8_t iteration, uint8_t index)
    {
        return valueHold[iteration][index];
    }
    void setMappedValue(float value, uint8_t iteration, uint8_t index)
    {
        valueHold[iteration][index] = value;
    }

    virtual size_t nullValue()
    {
        return impossible_index;
    }
};

class SDI12Device
{
protected:
    // SDI12 *sdi12;
    const String DEVICE_IDENTITY_ADDRESS = "SDI12_DEVICE_IDENTITY_";
    int randIdentity = -1;
    SDI12DeviceManager &manager = SDI12DeviceManager::getInstance();
    Bootstrap *boots;
    SDIParamElements *childElements;
    String serialMsgStr = "~R0!";
    Utils utils;
    const u_int8_t READ_FAILOVER_ATTEMPTS = 10;
    u_int8_t readAttempts = 0;
    void parseSerial(String ourReading);
    bool readyRead = false;
    bool readCompile = false;
    bool readReady();
    bool isConnected();
    u_int8_t maintenanceTick = 0;
    String ourReading = "";
    bool hasSerialIdentity();
    String constrictSerialIdentity();
    String serialResponseIdentity();
    String replaceSerialResponseItem(String message);
    bool validMessageString(String message);
    unsigned int READ_THRESHOLD = 12;
    size_t readAttempt = 0;
    int sendIdentity = -1;
    String fetchReading();
    void readWire();
    static const unsigned long WIRE_TIMEOUT = 1800;
    String getWire(String);
    void runSingleSample();
    String getCmd();
    String readSDI();
    void setupCloudFunctions();
    int setAddress(String address);
    void loadAddress();
    String getIdentityKey();
    String serialIdentity();

public:
    ~SDI12Device();
    SDI12Device(Bootstrap *, SDIParamElements *);
    SDI12Device(Bootstrap *);
    SDI12Device(Bootstrap *, int, SDIParamElements *);
    SDI12Device(Bootstrap *, int);
    String uniqueName();
    size_t readSize();
    void setElements(SDIParamElements *);
    void read();
    void loop();
    void clear();
    void print();
    void init();
    SDIParamElements *getElements();
    String name();
    void nullifyPayload(const char *key);
    u_int8_t maintenanceCount();
    u_int8_t paramCount();
    size_t buffSize();
    void restoreDefaults();
    Bootstrap *getBoots();
    void publish(JsonObject &writer, u_int8_t attempt_count);
    virtual float extractValue(float values[], size_t key);
    virtual float extractValue(float values[], size_t key, size_t max);
};

#endif