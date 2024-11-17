#include "resources/devices/device.h"

#ifndef turbidity_h
#define turbidity_h

class Turbidity : public Device
{
private:
    Bootstrap *boots;

public:
    virtual ~Turbidity();
    Turbidity();
    Turbidity(Bootstrap *boots);
    virtual String name();
    virtual void read();
    virtual void loop();
    virtual uint8_t maintenanceCount();
    virtual uint8_t paramCount();
    virtual void clear();
    virtual void print();
    virtual size_t buffSize();
    virtual void init();
    virtual void restoreDefaults();
    virtual void publish(JsonObject &writer, uint8_t attempt_count);
};

#endif