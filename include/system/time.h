#ifndef time_h
#define time_h

#include <ArduinoHttpClient.h>
#include "time.h"
#include <Arduino.h>
#include <ArduinoJson.h>
#include "SSLClientESP32.h"
#include "Managers.h"
#include "Connections.h"

#define TIME_FORMAT_ISO8601_FULL "%Y-%m-%dT%H:%M:%S%z"
class TimeClass
{
private:
    bool synced = false;
    unsigned long getTimeFromHttp(Connection &);
    unsigned long getTimeFromNTP(Connection &);
    const char *serverAddress = "worldtimeapi.org";
    const char *resource = "/api/timezone/Etc/UTC";
    const char *ntpServer = "pool.ntp.org";
    long gmtOffset_sec = 0;
    const int daylightOffset_sec = 3600;
    void setSystemTime(unsigned long unixTime);
    void setTimezone(float timezoneOffset);
    FileManager fm;
    float tz = 0;
    const uint8_t UPDATE_ATTEMPS = 1;
    void updateSystemTime(const struct tm &, float);

public:
    TimeClass();
    void zone(float);
    bool isSynced();
    String format(String, const char *);
    String format(unsigned long, const char *);
    String format(const char *);
    unsigned long now();
    bool init(Connection &);
    float tzOffset();
    bool parseMessage(const char *);
};

extern TimeClass Time;
#endif