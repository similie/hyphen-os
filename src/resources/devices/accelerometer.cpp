#include "accelerometer.h"
#include "device.h"
#include <Wire.h>
#include <ArduinoJson.h>
#include <math.h>

// Constructor: initialize sample index and pass the Bootstrap pointer to the base class.
Accelerometer::Accelerometer(Bootstrap *boots) : Device(boots)
{
    sampleIndex = 0;
    readyToRead = false;
    boots = boots;
}

Accelerometer::~Accelerometer()
{
}

// init() is called at setup. It powers up the sensor, initializes I2C, and configures the device.
void Accelerometer::init()
{
    // Power the sensor by pulling POW_PIN high.
    // pinMode(POW_PIN, OUTPUT);
    // digitalWrite(POW_PIN, HIGH);

    // Initialize I2C (ESP32 default SDA=21, SCL=22)
    // Wire.
    // Wire.begin();
    // Verify device by reading WHO_AM_I (should return 0x33)
    uint8_t who = readRegister(WHO_AM_I);
    Utils::log("Accelerometer WHO_AM_I: 0x", String(who, HEX));
    if (who != 0x33)
    {
        readyToRead = false;
        return Utils::log("Accelerometer not detected!: 0x", String(who, HEX));
        // Optionally, handle error here.
    }

    readyToRead = true;

    // Configure sensor for acceleration:
    // CTRL_REG1: 0x57 -> Enable all axes at 100Hz in normal mode.
    writeRegister(CTRL_REG1, 0x57);
    // CTRL_REG4: 0x88 -> Enable high-resolution mode with ±2g full scale.
    writeRegister(CTRL_REG4, 0x88);
    // Enable the internal temperature sensor:
    // TEMP_CFG_REG: setting bits 7 and 6 (0xC0) enables the sensor.
    writeRegister(TEMP_CFG_REG, 0xC0);

    // Reset the sample buffer.
    sampleIndex = 0;
    // applyCalibration();
}

void Accelerometer::applyCalibration()
{
    setFunction();
    Utils::log("ACCELEROMETER_BOOT_ADDRESS REGISTRATION", this->name());
    saveAddressForAcc = boots->registerAddress(this->name(), sizeof(AccStruct));
    Utils::log("ACCELEROMETER_BOOT_ADDRESS " + this->name(), String(saveAddressForAcc));
    this->config = getProm();
    if (!Utils::validConfigIdentity(this->config.version))
    {
        restoreDefaults();
    }
    configSetup();
}

void Accelerometer::restoreDefaults()
{
    this->config.tempOffset = 0;
    saveEEPROM(this->config);
}

AccStruct Accelerometer::getProm()
{
    AccStruct prom;
    Persist.get(saveAddressForAcc, prom);
    return prom;
}

void Accelerometer::saveEEPROM(AccStruct storage)
{
    if (saveAddressForAcc == 0)
    {
        return;
    }
    storage.version = 1;
    Persist.put(saveAddressForAcc, storage);
}

void Accelerometer::configSetup()
{
    tempOffset = config.tempOffset;
}

int Accelerometer::setTemperatureOffset(String offset)
{

    tempOffset = Utils::parseCloudFunctionInteger(offset, "Accelerometer temp offset"); // offset.toInt();
    return tempOffset;
}

void Accelerometer::setFunction()
{
    Hyphen.function("setTemperatureOffset", &Accelerometer::setTemperatureOffset, this);
}

// read() is called at an interval (derived from your publish frequency).
// It reads the current accelerometer and temperature data and stores it in the buffer.
void Accelerometer::read()
{

    if (!readyToRead || sampleIndex >= SAMPLE_BUFFER_SIZE)
    {
        return;
    }
    Serial.println("STARTING");
    int16_t x, y, z;
    readAccelData(x, y, z);
    Serial.println("READ");
    float temp = readTemperature();
    Serial.println("TEMP");
    readings[sampleIndex].ax = x;
    readings[sampleIndex].ay = y;
    readings[sampleIndex].az = z;
    readings[sampleIndex].temperature = temp;
    Serial.println("APPLIED");
    Serial.println(sampleIndex);
    sampleIndex++;
}

void Accelerometer::clearWriter(JsonObject &writer)
{
    writer["avg_x"] = 0;
    writer["avg_y"] = 0;
    writer["avg_z"] = 0;
    writer["avg_temp"] = 0;
    writer["avg_mag"] = 0;
    writer["max_mag"] = 0;
    writer["min_mag"] = 0;
    // writer["sample_count"] = 0;
}

// publish() is called at the publish interval (1–15 minutes).
// It aggregates the buffered data (e.g., averages, min/max values) and writes a JSON payload.
void Accelerometer::publish(JsonObject &writer, uint8_t attempt_count)
{
    if (!readyToRead || sampleIndex == 0)
    {
        return clearWriter(writer);
    }

    long sum_x = 0, sum_y = 0, sum_z = 0;
    float sum_temp = 0;
    float sum_mag = 0;
    float max_mag = -10000;
    float min_mag = 10000;
    for (size_t i = 0; i < sampleIndex; i++)
    {
        sum_x += readings[i].ax;
        sum_y += readings[i].ay;
        sum_z += readings[i].az;
        sum_temp += readings[i].temperature;

        float mag = sqrt((float)readings[i].ax * readings[i].ax +
                         (float)readings[i].ay * readings[i].ay +
                         (float)readings[i].az * readings[i].az);
        sum_mag += mag;
        if (mag > max_mag)
        {
            max_mag = mag;
        }
        if (mag < min_mag)
        {
            min_mag = mag;
        }
    }
    float avg_x = sum_x / (float)sampleIndex;
    float avg_y = sum_y / (float)sampleIndex;
    float avg_z = sum_z / (float)sampleIndex;
    float avg_temp = sum_temp / sampleIndex;
    float avg_mag = sum_mag / (float)sampleIndex;

    writer["avg_x"] = avg_x;
    writer["avg_y"] = avg_y;
    writer["avg_z"] = avg_z;
    writer["avg_temp"] = avg_temp;
    writer["avg_mag"] = avg_mag;
    writer["max_mag"] = max_mag;
    writer["min_mag"] = min_mag;
    // writer["sample_count"] = sampleIndex;

    // After publishing, clear the sample buffer.
    sampleIndex = 0;
}

// loop() is called continuously; for this device no background loop processing is required.
void Accelerometer::loop()
{
    // You could add non-blocking tasks here if needed.
}

// clear() resets the sample buffer.
void Accelerometer::clear()
{
    sampleIndex = 0;
}

// print() outputs the current stored samples (for debugging purposes).
void Accelerometer::print()
{
    Serial.println("Accelerometer Sample Buffer:");
    for (size_t i = 0; i < sampleIndex; i++)
    {
        float mag = sqrt((float)readings[i].ax * readings[i].ax +
                         (float)readings[i].ay * readings[i].ay +
                         (float)readings[i].az * readings[i].az);
        Serial.print("Sample ");
        Serial.print(i);
        Serial.print(": X=");
        Serial.print(readings[i].ax);
        Serial.print(" Y=");
        Serial.print(readings[i].ay);
        Serial.print(" Z=");
        Serial.print(readings[i].az);
        Serial.print(" Temp=");
        Serial.print(readings[i].temperature);
        Serial.print(" Mag=");
        Serial.println(mag);
    }
}

// name() returns the device name.
String Accelerometer::name()
{
    return "Accelerometer";
}

// paramCount() returns the number of parameters that will be published.
// In this example we publish 7 parameters.
uint8_t Accelerometer::paramCount()
{
    return 7;
}

// Helper: write a value to a sensor register over I2C.
void Accelerometer::writeRegister(uint8_t reg, uint8_t value)
{
    Wire.beginTransmission(ACCEL_I2C_ADDR);
    Wire.write(reg);
    Wire.write(value);
    Wire.endTransmission();
}

// Helper: read a single byte from a sensor register.
uint8_t Accelerometer::readRegister(uint8_t reg)
{
    Wire.beginTransmission(ACCEL_I2C_ADDR);
    Wire.write(reg);
    Wire.endTransmission(false); // Send a repeated start
    int byteRequest = 1;
    Wire.requestFrom(ACCEL_I2C_ADDR, byteRequest);
    return Wire.read();
}

// Helper: read 6 bytes starting at OUT_X_L (with auto-increment enabled)
// to get the 12-bit acceleration data for X, Y, and Z axes.
void Accelerometer::readAccelData(int16_t &x, int16_t &y, int16_t &z)
{
    Wire.beginTransmission(ACCEL_I2C_ADDR);
    Wire.write(OUT_X_L | 0x80); // Set auto-increment bit.
    Wire.endTransmission(false);
    Wire.requestFrom(ACCEL_I2C_ADDR, 6);
    uint8_t xl = Wire.read();
    uint8_t xh = Wire.read();
    uint8_t yl = Wire.read();
    uint8_t yh = Wire.read();
    uint8_t zl = Wire.read();
    uint8_t zh = Wire.read();
    // The raw data is left-justified 12-bit; shift right by 4 bits.
    x = (((int16_t)xh << 8) | xl) >> 4;
    y = (((int16_t)yh << 8) | yl) >> 4;
    z = (((int16_t)zh << 8) | zl) >> 4;
}

// Helper: read temperature from the internal sensor.
// Reads two bytes from OUT_TEMP_L and OUT_TEMP_H, then shifts to obtain a 10-bit value.
// The datasheet indicates roughly 1 LSB per °C (calibration may be needed).
float Accelerometer::readTemperature()
{
    Wire.beginTransmission(ACCEL_I2C_ADDR);
    Wire.write(OUT_TEMP_L | 0x80);
    Wire.endTransmission(false);
    Wire.requestFrom(ACCEL_I2C_ADDR, 2);
    uint8_t tempL = Wire.read();
    uint8_t tempH = Wire.read();
    int16_t rawTemp = ((int16_t)tempH << 8) | tempL;
    // Shift right by 6 to get the 10-bit value.
    rawTemp = rawTemp >> 6;
    // float temperature = (float)rawTemp; // Adjust offset/calibration as needed.
    float temperature = (float)rawTemp + (-25.0 + tempOffset);
    return temperature;
}