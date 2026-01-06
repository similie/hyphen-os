#ifndef ACCELEROMETER_H
#define ACCELEROMETER_H

#include "device.h"
#include <ArduinoJson.h>

// Define the power pin (must be pulled high) per your requirement.
#define POW_PIN 27

// I2C address for the LIS2DH12 (adjust if SDO is tied high to Vdd)
#define ACCEL_I2C_ADDR 0x18

// Register definitions from the datasheet
#define WHO_AM_I 0x0F
#define CTRL_REG1 0x20
#define CTRL_REG3 0x22
#define CTRL_REG4 0x23
#define TEMP_CFG_REG 0x1F
#define OUT_TEMP_L 0x0C
#define OUT_TEMP_H 0x0D
#define OUT_X_L 0x28

// Define the sample buffer size. (You may use Constants::OVERFLOW_VAL if defined)
#define SAMPLE_BUFFER_SIZE 15

// Structure to hold one sensor reading.
struct AccelReading
{
    int16_t ax;
    int16_t ay;
    int16_t az;
    float temperature;
};

struct AccStruct
{
    uint8_t version;
    int tempOffset;
    // int pin;
};

class Accelerometer : public Device
{
public:
    Accelerometer(Bootstrap *boots);
    virtual ~Accelerometer();

    // Overridden Device functions:
    virtual void init();
    virtual void read();
    virtual void publish(JsonObject &writer, uint8_t attempt_count, const String &payloadId);
    virtual void loop();
    virtual void clear();
    virtual void print();
    virtual String name();
    virtual uint8_t paramCount();

private:
    // Buffer for sensor samples.
    Bootstrap *boots;
    AccelReading readings[Constants::OVERFLOW_VAL];
    bool readyToRead = false;
    size_t sampleIndex;
    int tempOffset = 0;
    void setFunction();
    int setTemperatureOffset(String);
    uint16_t saveAddressForAcc = 0;
    AccStruct config;
    // Helper functions to communicate with the sensor.
    AccStruct getProm();
    void applyCalibration();
    void restoreDefaults();
    void configSetup();
    void saveEEPROM(AccStruct storage);
    void writeRegister(uint8_t reg, uint8_t value);
    uint8_t readRegister(uint8_t reg);
    void readAccelData(int16_t &x, int16_t &y, int16_t &z);
    float readTemperature();
    void clearWriter(JsonObject &);
};

#endif // ACCELEROMETER_H