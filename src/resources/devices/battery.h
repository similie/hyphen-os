#include "Hyphen.h"
#include "string.h"
#include "device.h"
#include "math.h"
#include "resources/bootstrap/bootstrap.h"
#include <stdint.h>
#ifndef battery_h
#define battery_h

class Battery : public Device
{
private:
    Bootstrap *boots;
    const char *percentname = "bat";
    const char *voltsname = "b_v";
    const uint8_t PARAM_LENGTH = 2;
    String deviceName = "Battery";
    FuelGaugeClass fuel;

public:
    ~Battery();
    Battery();
    Battery(Bootstrap *boots);
    void read();
    void loop();
    void clear();
    void print();
    void init();
    String name();
    uint8_t maintenanceCount();
    uint8_t paramCount();
    size_t buffSize();
    void restoreDefaults();
    float getVCell();
    float getNormalizedSoC();
    inline float batteryCharge();
    void publish(JsonObject &writer, uint8_t attempt_count);
};

#endif