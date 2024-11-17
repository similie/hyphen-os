#include "system/time.h"
TimeClass::TimeClass()
{
}

void TimeClass::zone(float timezone)
{
    gmtOffset_sec = (long)(timezone * 3600); // Converts hours to seconds
}

String TimeClass::format(String currentStringTime, const char *formatVal)
{
    unsigned long unixTime = currentStringTime.toInt();
    return format(unixTime, formatVal); // Calls the other format function
}

String TimeClass::format(unsigned long unixTime, const char *format)
{
    time_t rawTime = static_cast<time_t>(unixTime);
    struct tm *timeinfo = localtime(&rawTime);

    if (timeinfo == nullptr)
    {
        Serial.println("Failed to obtain time information.");
        return String(); // Return an empty string if timeinfo is null
    }

    char buffer[80];
    strftime(buffer, sizeof(buffer), format, timeinfo);
    return String(buffer);
}

String TimeClass::format(const char *formatString)
{
    return format(now(), formatString); // Returns empty string if time retrieval fails
}

unsigned long TimeClass::now()
{
    struct tm timeinfo;
    if (getLocalTime(&timeinfo))
    {
        return mktime(&timeinfo); // Converts to Unix timestamp
    }
    Serial.println("Failed to obtain time"); // Prints error message to serial monitor
    return 0;                                // Returns 0 if time retrieval fails
}

bool TimeClass::init()
{
    Serial.println("Waiting for NTP time sync: ");
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

    struct tm timeinfo;
    int retryCount = 10; // Number of retries
    while (!getLocalTime(&timeinfo) && retryCount > 0)
    {
        Serial.println("Waiting for NTP time sync...");
        delay(500); // Delay between retries
        retryCount--;
    }

    if (retryCount == 0)
    {
        Serial.println("Failed to obtain NTP time");
        return false;
    }

    Serial.println("NTP time synchronized");
    return true;
}