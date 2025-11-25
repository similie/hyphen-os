#ifndef time_h
#define time_h

#ifndef DEFAULT_TIMEZONE
#define DEFAULT_TIMEZONE 9.0
#endif

#include <ArduinoHttpClient.h>
#include "time.h"
#include <Arduino.h>
#include <ArduinoJson.h>
#include "SSLClientESP32.h"
#include "Managers.h"
#include "Connections.h"
#include "system/persistence.h"

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
    float tz = DEFAULT_TIMEZONE;
    const uint8_t UPDATE_ATTEMPS = 1;
    void updateSystemTime(const struct tm &, float);
    Persistence persist;
    const unsigned long MAX_ACCEPTABLE_AGE_SEC = 24 * 3600; // 24 hours
    const unsigned long MAX_DRIFT_SEC = 60;                 // 1 minute
    unsigned long lastRestoreTimeUnix = 0;
    unsigned long lastRestoreUptimeMs = 0;
    bool volatileTimeRestored = false;

public:
    TimeClass();
    void zone(float);
    bool isSynced();
    bool hasTime();
    String format(String, const char *);
    String format(unsigned long, const char *);
    String format(const char *);
    unsigned long now();
    bool init(Connection &);
    float tzOffset();
    bool parseMessage(const char *);
    bool storeTimeToPersist();
    bool restoreTimeFromPersist();
};

extern TimeClass Time;
#endif