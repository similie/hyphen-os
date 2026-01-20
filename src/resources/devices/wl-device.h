#pragma once

#include "Hyphen.h"
#include "device.h"
#include "resources/bootstrap/bootstrap.h"
#include "resources/utils/utils.h"
#include <stdint.h>
#include <math.h>

#define DIGITAL_DEFAULT true
#define DEF_DISTANCE_READ_DIG_CALIBRATION 0.01724137931
#define DEF_DISTANCE_READ_AN_CALIBRATION 0.335

#define DIG_PIN 13 // D3  blue off port3
#define AN_PIN 1   // stripe blue line off port0

const size_t WL_PARAM_SIZE = 1;

struct WLStruct
{
    uint8_t version;
    double calibration;
    char digital; // 'y' / 'n'
};

class WlDevice : public Device
{
private:
    Bootstrap *boots = nullptr;
    uint32_t lastGoodPwUs = 0;
    // config / identity
    WLStruct config{};
    uint16_t saveAddressForWL = 0;
    int sendIdentity = -1;

    // pin + mode
    int readPin = -1; // override pin; if -1 use DIG_PIN/AN_PIN based on mode
    bool digital = DIGITAL_DEFAULT;

    // calibration
    double currentCalibration = (DIGITAL_DEFAULT) ? DEF_DISTANCE_READ_DIG_CALIBRATION : DEF_DISTANCE_READ_AN_CALIBRATION;

    // storage + stats
    Utils utils;
    uint8_t maintenanceTick = 0;

    String deviceName = "wl";
    String readParams[WL_PARAM_SIZE] = {"dist"};
    const size_t PARAM_LENGTH = sizeof(readParams) / sizeof(String);
    int VALUE_HOLD[sizeof(readParams) / sizeof(String)][Constants::OVERFLOW_VAL];

    // --- internal helpers ---
    int getPin();
    void setPinMode();
    void configSetup();
    void restoreDefaults();

    // EEPROM
    WLStruct getProm();
    void saveEEPROM(WLStruct storage);

    // cloud + identity
    bool hasSerialIdentity();
    String appendIdentity();
    String uniqueName();
    void setCloudFunctions();
    String getParamName(size_t index);

    // config conversion
    char setDigital(bool value);
    bool isDigital(char value);
    bool isSaneCalibration(double cal, bool digitalMode);

    // reading
    uint32_t readPulseUs(uint32_t timeoutUs);
    uint32_t getReadValue();
    uint32_t readWL();

    // cloud setters
    int setDigitalCloud(String read);
    int setCalibration(String read);

public:
    ~WlDevice();

    WlDevice(Bootstrap *boots);
    WlDevice(Bootstrap *boots, int sendIdentity);
    WlDevice(Bootstrap *boots, int sendIdentity, int readPin);

    String name();
    void init();
    void read();
    void loop();
    void clear();
    void print();

    void publish(JsonObject &writer, uint8_t attempt_count, const String &payloadId);
    uint8_t maintenanceCount();
    uint8_t paramCount();
    size_t buffSize();
};
// #include "Hyphen.h"
// #include "string.h"
// #include "device.h"
// #include <stdint.h>
// #include "resources/bootstrap/bootstrap.h"
// #include "resources/utils/utils.h"
// #include "math.h"
// #include <driver/gpio.h>
// #include <driver/rmt.h>
// #define DIGITAL_DEFAULT true
// #define DEF_DISTANCE_READ_DIG_CALIBRATION 0.01724137931
// #define DEF_DISTANCE_READ_AN_CALIBRATION 0.335
// #ifndef wl_device_h
// #define wl_device_h
// #define DIG_PIN 13 // D3  blue off port3
// #define AN_PIN 1   // stripe blue line off port0

// const size_t WL_PARAM_SIZE = 1;

// struct WLStruct
// {
//     uint8_t version;
//     double calibration;
//     char digital;
//     // int pin;
// };

// class WlDevice : public Device
// {
// private:
//     volatile uint32_t riseUs = 0;
//     volatile uint32_t lastPwUs = 0;
//     volatile uint32_t lastPwAtUs = 0;
//     volatile bool havePw = false;
//     // int wlGpio = -1;
//     volatile uint32_t edgeCount = 0;
//     uint32_t lastReattachMs = 0;
//     uint8_t consecutiveZeroReads = 0;
//     portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
//     bool isPulseStale(uint32_t nowUs, uint32_t maxAgeUs);
//     static void IRAM_ATTR isrThunk(void *arg)
//     {
//         ((WlDevice *)arg)->handleEdge();
//     }
//     void suspendCapture();
//     void resumeCapture();
//     void reattachInterrupt();
//     uint32_t capturePulseBlocking(uint32_t timeoutUs);
//     void IRAM_ATTR handleEdge();
//     int readPin = -1;
//     bool isSaneCalibration(double cal, bool digitalMode);
//     bool digital = DIGITAL_DEFAULT;
//     int getPin();
//     uint32_t getReadValue();
//     uint32_t readPulseUs();
//     uint16_t saveAddressForWL = 0;
//     double currentCalibration = (DIGITAL_DEFAULT) ? DEF_DISTANCE_READ_DIG_CALIBRATION : DEF_DISTANCE_READ_AN_CALIBRATION;
//     void configSetup();
//     void restoreDefaults();
//     bool isDigital();
//     int setDigitalCloud(String read);
//     int setCalibration(String read);
//     void saveEEPROM(WLStruct storage);
//     String appendIdentity();
//     WLStruct config;
//     String uniqueName();
//     bool hasSerialIdentity();
//     void setCloudFunctions();
//     String getParamName(size_t index);
//     Bootstrap *boots;
//     String deviceName = "wl";
//     int sendIdentity = -1;
//     // String readParams[WL_PARAM_SIZE] = {"wl_pw", "hydrometric_level"};
//     //  String readParams[WL_PARAM_SIZE] = {"wl_pw"}; // water tank  or wl
//     String readParams[WL_PARAM_SIZE] = {"dist"}; // river level or hydrometric level

//     Utils utils;
//     uint8_t maintenanceTick = 0;
//     uint32_t readWL();
//     const size_t PARAM_LENGTH = sizeof(readParams) / sizeof(String);
//     int VALUE_HOLD[sizeof(readParams) / sizeof(String)][Constants::OVERFLOW_VAL];

//     enum
//     {
//         // wl_pw
//         hydrometric_level
//         // wl_cm
//     };

// public:
//     ~WlDevice();
//     WlDevice(Bootstrap *boots);
//     WlDevice(Bootstrap *boots, int sendIdentity);
//     WlDevice(Bootstrap *boots, int sendIdentity, int readPin);
//     char setDigital(bool value);
//     bool isDigital(char value);
//     void setPin(bool digital);
//     void read();
//     WLStruct getProm();
//     String name();
//     void loop();
//     void clear();
//     void print();
//     void init();
//     uint8_t maintenanceCount();
//     uint8_t paramCount();
//     size_t buffSize();
//     void publish(JsonObject &writer, uint8_t attempt_count, const String &payloadId);
// };

// #endif