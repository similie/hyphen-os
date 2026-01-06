#include "Hyphen.h"
#include "string.h"
#include <stdint.h>
#include "device.h"
#include "resources/bootstrap/bootstrap.h"
#include "resources/processors/LocalProcessor.h"
#include "resources/utils/utils.h"

#define DEFAULT_TIP_SIZE 0.2

// #define RAIN_GAUGE_PIN D7 // Blue when using Port1
// #define RAIN_GAUGE_PIN A1 // Stripe Blue when using Port1
#define RAIN_GAUGE_PIN D3 // Blue when using Port3

#ifndef rain_gauge_h
#define rain_gauge_h

struct RainGaugeStruct
{
    uint8_t version;
    double calibration;
};

class RainGauge : public Device
{
private:
    volatile bool interruptTrpped = false;
    uint16_t eepromAddress = 0;
    int counts = 0;
    uint8_t errorCount = 0;
    double perTipMultiple = DEFAULT_TIP_SIZE; // mm
    Bootstrap *boots;
    RainGaugeStruct config;
    void setPerTipMultiple();
    bool validAddress();
    String valueMap[1] =
        {
            "pre"};
    enum
    {
        precipitation
    };
    String deviceName = "RainGauge";
    void pullStoredConfig();
    int setTipMultiple(String value);
    void cloudFunctions();
    void setPin();
    void countChange();
    void setInterrupt();
    void reqestAddress();

public:
    ~RainGauge();
    RainGauge(Bootstrap *boots);
    String name();
    void read();
    void loop();
    uint8_t maintenanceCount();
    uint8_t paramCount();
    void clear();
    void print();
    size_t buffSize();
    void init();
    void restoreDefaults();
    void publish(JsonObject &writer, uint8_t attempt_count, const String &payloadId);
};

#endif