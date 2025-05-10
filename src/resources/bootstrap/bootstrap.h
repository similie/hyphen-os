
#ifndef bootstrap_h
#define bootstrap_h
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "Hyphen.h"
#include "resources/utils/battery.h"
#include "resources/utils/utils.h"
#define PRINT_MEMORY 1
#define STORAGE_SIZE 8197
static const int TIMEZONE = TIMEZONE_SET;

struct DeviceConfig
{
    uint8_t version;
    byte device[DEVICE_BYTE_BUFFER_SIZE];
};

struct DeviceMetaStruct
{
    uint8_t version;
    uint8_t count;
};

struct DeviceStruct
{
    uint8_t version;
    uint16_t size;
    unsigned long name;
    // const char * name;
    uint16_t address;
};

struct EpromStruct
{
    uint8_t version;
    uint8_t pub;
    int timezone;
    double sleep;
    int lowPowerMode;
};

struct MetaStruct
{
    uint8_t version;
    uint8_t count;
};

class Bootstrap
{
private:
    TaskHandle_t timeSyncHandle = nullptr;
    int localTimezone = TIMEZONE;
    bool bootstrapped = false;
    uint8_t publicationIntervalInMinutes = DEFAULT_PUB_INTERVAL;
    double batterySleepThresholdValue = 0;

    int publishedInterval = DEFAULT_PUB_INTERVAL;
    int lowPowerMode = 0;
    int lowPowerBeat = 1;

    String processorName = "";
    bool validTimezone(int);
    void applyTimeZone();
    void pullRegistration();
    void addNewDeviceToStructure(DeviceStruct);
    void collectDevices();
    void setFunctions();
    int setMaintenanceMode(String);
    int setBatterySleepThreshold(String);
    int setTimeZone(String);
    void sendBatteryValueToConfig(double);
    int maxAddressIndex();

    DeviceMetaStruct deviceMeta;
    DeviceStruct devices[MAX_DEVICES];
    DeviceStruct getDeviceByName(String, uint16_t);
    void saveDeviceMetaDetails();
    uint16_t getNextDeviceAddress();
    uint16_t deviceInitAddress();
    void processRegistration();
    bool maintenaceMode = false;
    void bootstrap();
    EpromStruct getsavedConfig();
    void putSavedConfig(EpromStruct);
    void sendTimezoneValueToConfig(int);
    void setMetaAddresses();
    const size_t MAX_VALUE_THRESHOLD = MAX_SEND_TIME;
    unsigned int READ_TIMER;
    unsigned int PUBLISH_TIMER;
    bool strappingTimers = false;

    uint16_t deviceMetaAddresses[MAX_DEVICES];
    uint16_t deviceConfigAddresses[MAX_DEVICES];
    // BEACH Storage
    static const uint16_t START_ADDRESS = sizeof(EpromStruct) + 8;
    // Device Meta Storage
    const uint16_t DEVICE_META_ADDRESS = START_ADDRESS + sizeof(MetaStruct) + 8;
    // Stores count details
    const uint16_t DEVICE_CONFIG_STORAGE_META_ADDRESS = DEVICE_META_ADDRESS + sizeof(DeviceMetaStruct) + 8;
    // Device Type Storage
    const uint16_t DEVICE_HOLD_ADDRESS = DEVICE_CONFIG_STORAGE_META_ADDRESS + (sizeof(DeviceStruct) * MAX_DEVICES) + (MAX_DEVICES + 2) + 8;
    // Now the storage for the specific devices
    const uint16_t DEVICE_SPECIFIC_CONFIG_ADDRESS = DEVICE_HOLD_ADDRESS + ((sizeof(DeviceConfig) + 4) * MAX_DEVICES) + (MAX_DEVICES + 2);
    // in case we have to manually issue an address on first come first serve.
    uint16_t manualDeviceTracker = DEVICE_SPECIFIC_CONFIG_ADDRESS;

public:
    ~Bootstrap();
    void timers();
    bool isStrapped();
    void init();
    String getProcessorName();
    bool doesNotExceedsMaxAddressSize(uint16_t);
    void strapDevices(String *);
    void storeDevice(String, int);
    void clearDeviceConfigArray();
    void haultPublication();
    void resumePublication();
    void restoreDefaults();
    void setPublishTimer(bool);
    bool isTimerUpdateReady();
    void endTimerCheck();
    void setHeartbeatTimer(bool);
    void setReadTimer(bool);
    void buildSendInterval(int);
    void buildSleepThreshold(double);
    void setMaintenance(bool);
    bool hasMaintenance();
    bool publishTimerFunc();
    bool heartbeatTimerFunc();
    bool readTimerFun();
    bool printMemoryFunc();
    void printMem();
    bool checkOfflineReady();
    void resetOfflineCheck();
    bool lowPowerCheck();
    void resetPowerCheck();
    int setLowPowerMode(String);
    void applyLowPowerMode(int);
    void sendLowPowerModeToConfig(int);
    int getLowPowerModeTime();

    void timeCheckPoll();
    unsigned int getReadTime();
    unsigned int getPublishTime();
    uint16_t registerAddress(String, uint16_t);
    static size_t storageSize();
    size_t getMaxVal();
    String removeNewLine(String value);
    // static void syncTimeTask(void *param);
};

#endif