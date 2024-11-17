#include "Hyphen.h"
#include "string.h"
#ifndef heartbeat_h
#define heartbeat_h

#define HAS_LOCAL_POWER false
#define HEART_BUFFER_SIZE 400

class HeartBeat
{
private:
    // char buf[HEART_BUFFER_SIZE];
    String deviceID;

    String cellAccessTech(int rat);
    void setCellDeets(JsonObject &writer);
    void setPowerDeets(JsonObject &writer);
    void setSystemDeets(JsonObject &writer);

public:
    ~HeartBeat();
    HeartBeat(String deviceID);
    String pump();
};

#endif