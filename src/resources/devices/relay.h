#include "Hyphen.h"
#include "string.h"
#include "device.h"
#include <stdint.h>
#include "resources/bootstrap/bootstrap.h"
#include "resources/processors/LocalProcessor.h"

#ifndef relay_h
#define relay_h
#define DEFAULT_RELAY_PIN 7
#define ON_BOARD_RELAY_PIN 2
class Relay : public Device
{
private:
    bool registered = false;
    Bootstrap *boots;
    int pin = DEFAULT_RELAY_PIN;
    bool invert = false;
    bool isOn = false;
    uint8_t POW_ON = invert ? LOW : HIGH;
    uint8_t POW_OFF = invert ? HIGH : LOW;
    void buildOutputs();
    String sendIdentity = "";
    String appendIdentity();
    void setCloudFunctions();
    uint32_t runTime = 0;
    unsigned long timeRunning = 0;
    unsigned long startTime = 0;
    void shouldToggleOff();
    int turnOn(String);
    int turnOff(String);
    int toggle(String);
    unsigned long getStartDelta();
    int setInvert(String);

public:
    ~Relay();
    Relay();
    Relay(Bootstrap *);
    Relay(Bootstrap *, int);
    Relay(Bootstrap *, int, bool);
    Relay(Bootstrap *, int, String);
    Relay(Bootstrap *, int, String, bool);
    Relay(int);
    Relay(int, bool);
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
    bool on();
    bool off();
};

#endif