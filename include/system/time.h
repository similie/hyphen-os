#ifndef time_h
#define time_h

#include "time.h"
#include <Arduino.h>
#define TIME_FORMAT_ISO8601_FULL "%Y-%m-%dT%H:%M:%S%z"
class TimeClass
{
    const char *ntpServer = "pool.ntp.org";
    long gmtOffset_sec = 0;
    const int daylightOffset_sec = 3600;

public:
    TimeClass();
    void zone(float);
    String format(String, const char *);
    String format(unsigned long, const char *);
    String format(const char *);
    unsigned long now();
    bool init();
};
#endif