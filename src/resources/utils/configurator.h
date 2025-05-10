#include "Hyphen.h"
// #include "resources/devices/device-manager.h"
#include "resources/devices/device.h"
#include "resources/devices/all-weather.h"
#include "resources/devices/soil-moisture.h"
#include "resources/devices/wl-device.h"
#include "resources/devices/rain-gauge.h"
#include "resources/devices/gps-device.h"
#include "resources/devices/battery.h"
#include "resources/devices/flow-meter.h"
#include "resources/devices/video-capture.h"
#include "resources/devices/relay.h"
#include "resources/devices/accelerometer.h"
#include "utils.h"

#define CONFIG_STORAGE_MAX 5
#define CURRENT_DEVICES_COUNT 12

#define DEVICE_NAME_INDEX 0
#define DEVICE_IDENTITY_INDEX 1
#define DEVICE_PIN_INDEX 2
#define DEVICE_FUTURE_INDEX 3
#define DEVICE_FUTURE_2_INDEX 4

#ifndef configurator_h
#define configurator_h

class Configurator
{
private:
    String devicesAvailable[CURRENT_DEVICES_COUNT] = {"all_weather", "soil_moisture", "rain_gauge", "gps", "battery", "sonic_sensor", "flow_meter", "beco_flow_meter", "video_capture", "rika_airquality", "relay", "accelerometer"};
    enum
    {
        all_weather,
        soil_moisture,
        rain_gauge,
        gps_device,
        battery,
        sonic_sensor,
        flow_meter,
        beco_flow_meter,
        video_capture,
        rika_airquality,
        relay,
        accelerometer
    };
    Device *pullDeviceType(String configurationStore[], Bootstrap *boots);
    bool applyDevice(Device *d);
    bool noIdentity(String configurationStore[], int index);
    int parseIdentity(String value);

public:
    ~Configurator();
    Configurator();
    int getEnumIndex(String value);
    bool violatesOccurrences(String value, int occrances);
    void loadConfigurationStorage(String payload, String configurationStore[], size_t size);
    Device *addDevice(String payload, Bootstrap *boots);
    bool removeDevice(String payload);
};

#endif