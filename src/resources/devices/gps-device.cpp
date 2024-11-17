#include "gps-device.h"

GpsDevice::~GpsDevice()
{
}

GpsDevice::GpsDevice()
{
}

GpsDevice::GpsDevice(Bootstrap *boots)
{
    this->boots = boots;
}

String GpsDevice::name()
{
    return this->deviceName;
}

void GpsDevice::restoreDefaults()
{
}

void GpsDevice::init()
{
}

void GpsDevice::publish(JsonObject &writer, uint8_t attempt_count)
{
}

void GpsDevice::parseSerial(String ourReading)
{
    Serial.println(ourReading);
}

void GpsDevice::read()
{
    Serial1.println("$GPS_1");
}

void GpsDevice::loop()
{
}

void GpsDevice::clear()
{
}

void GpsDevice::print()
{
}

size_t GpsDevice::buffSize()
{
    return 150;
}

uint8_t GpsDevice::paramCount()
{
    return 4;
}

uint8_t GpsDevice::maintenanceCount()
{
    return 0;
}
