#include "system/time.h"

extern const uint8_t ca_cert_bundle_start[] asm("_binary_data_crt_x509_crt_bundle_bin_start");
extern const uint8_t ca_cert_bundle_end[] asm("_binary_data_crt_x509_crt_bundle_bin_end");

TimeClass::TimeClass()
{
    persist.begin("hyphen_time");
}

float TimeClass::tzOffset()
{
    return tz;
}

bool TimeClass::storeTimeToPersist()
{
    if (!isSynced())
    {
        return false;
    }
    unsigned long currentUnix = now(); // your class’s now(), returns 0 if failed
    if (currentUnix == 0)
    {
        // no valid time to store
        return false;
    }
    unsigned long uptimeMs = millis(); // or esp_timer_get_time()/1000

    persist.put("lastUnixTime", currentUnix);
    persist.put("lastUptimeMs", uptimeMs);
    persist.put("timeValid", true);
    Log.noticeln("Time stored for persist: unix=%lu, uptimeMs=%lu", currentUnix, uptimeMs);

    return true;
}
bool TimeClass::restoreTimeFromPersist()
{
    bool timeValid = false;
    persist.get("timeValid", timeValid);
    if (!timeValid)
    {
        synced = false;
        return false;
    }
    persist.put("timeValid", false); // Invalidate after reading
    unsigned long storedUnix = 0;
    persist.get("lastUnixTime", storedUnix);
    unsigned long storedUptimeMs = 0;
    persist.get("lastUptimeMs", storedUptimeMs);

    if (storedUnix == 0 || storedUptimeMs == 0)
    {
        synced = false;
        return false;
    }

    unsigned long nowUptimeMs = millis();
    unsigned long deltaMs = nowUptimeMs - storedUptimeMs;
    unsigned long deltaSec = deltaMs / 1000;
    unsigned long approxUnix = storedUnix + deltaSec;

    // Apply approximate time to system
    setSystemTime(approxUnix);
    synced = false;
    volatileTimeRestored = true;
    lastRestoreTimeUnix = approxUnix;
    lastRestoreUptimeMs = nowUptimeMs;

    Log.noticeln("Time restored from persist: approxUnix=%lu (stored=%lu + delta %lu ms)",
                 approxUnix, storedUnix, deltaMs);

    // 1) Check age of stored timestamp
    unsigned long ageSeconds = approxUnix - storedUnix;
    if (ageSeconds > MAX_ACCEPTABLE_AGE_SEC)
    {
        Log.warningln("Restored time is older than tolerance (%lu sec > %lu sec)",
                      ageSeconds, (unsigned long)MAX_ACCEPTABLE_AGE_SEC);
        synced = false;
        return false;
    }

    // 2) Check drift potential
    // define a conservative worst‐case drift rate, e.g., 100 ppm = 0.0001
    const float worstCaseDriftRate = 0.0001f; // i.e., 100 ppm
    float maxDriftSec = deltaSec * worstCaseDriftRate;
    if (maxDriftSec > MAX_DRIFT_SEC)
    {
        Log.warningln("Possible drift exceeds tolerance (~%.2f sec > %lu sec)",
                      maxDriftSec, (unsigned long)MAX_DRIFT_SEC);
        synced = false;
        return false;
    }

    // If we get here, the restored time is plausible
    return true;
}

void TimeClass::zone(float timezone)
{
    tz = timezone;
    gmtOffset_sec = (long)(tz * 3600); // Converts hours to seconds
}

bool TimeClass::isSynced()
{
    return synced;
}

bool TimeClass::hasTime()
{
    return synced || volatileTimeRestored;
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

void TimeClass::setSystemTime(unsigned long unixTime)
{
    struct timeval tv;
    tv.tv_sec = unixTime;
    tv.tv_usec = 0; // Microseconds
    settimeofday(&tv, nullptr);
    setTimezone(gmtOffset_sec / 3600);
}

void TimeClass::setTimezone(float timezoneOffset)
{
    int hours = static_cast<int>(timezoneOffset);
    int minutes = static_cast<int>((timezoneOffset - hours) * 60);
    char tz[10];
    snprintf(tz, sizeof(tz), "UTC%+03d:%02d", -hours, minutes); // POSIX reversed sign
    // snprintf(tz, sizeof(tz), "UTC-%02d:%02d", hours, minutes); // Reverse the sign
    Serial.printf("Setting timezone to %s\n", tz);
    setenv("TZ", tz, 1);
    tzset();
}

void TimeClass::updateSystemTime(const struct tm &timeinfo, float timezoneOffset)
{
    struct tm mutableTimeinfo = timeinfo;
    // Adjust the time based on the timezone offset
    time_t rawTime = mktime(&mutableTimeinfo);
    if (rawTime == -1)
    {
        Serial.println("Failed to convert timeinfo to time_t");
        return;
    }
    // @todo verify this is correct in indo
    // rawTime += static_cast<time_t>(timezoneOffset * 3600); // Adjust for timezone
    // Set the system time
    struct timeval now;
    now.tv_sec = rawTime;
    now.tv_usec = 0;
    if (settimeofday(&now, nullptr) != 0)
    {
        Log.warningln("Failed to set system time");
        return;
    }

    // Configure the timezone environment variable
    setTimezone(timezoneOffset);
    Log.noticeln("System time updated successfully");
}

bool TimeClass::init(Connection &connection)
{
    Log.noticeln("Waiting for NTP time sync: ");
    uint8_t retryCount = UPDATE_ATTEMPS;
    synced = false;
    for (retryCount; retryCount >= 0; retryCount--)
    {
        if (!connection.isConnected())
        {
            Log.errorln("Connection not available");
            break;
        }

        struct tm timeinfo;
        // float timezone;
        float tzValue = 0;
        if (connection.getTime(timeinfo, tzValue))
        {
            updateSystemTime(timeinfo, tz);
            synced = true;
            return synced;
        }

        configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

        if (getLocalTime(&timeinfo))
        {
            synced = true;
            return synced;
        }
        // try to get time from ntp HERE:
        unsigned long unixTime = getTimeFromNTP(connection); // @todo:: correct
        if (unixTime > 0)
        {
            Serial.printf("NTP sync successful, setting using NTP to: %lu\n", unixTime);
            setSystemTime(unixTime); // Add one hour to compensate for UTC
            synced = true;
            return synced;
        }
        // we desperately fallback to http
        unixTime = getTimeFromHttp(connection);
        if (unixTime > 0)
        {
            Serial.printf("NTP sync successful, setting time to: %lu\n", unixTime);
            setSystemTime(unixTime); // Add one hour to compensate for UTC
            synced = true;
            return synced;
        }
        vTaskDelay(pdMS_TO_TICKS(100)); // Delay between retries
    }

    if (!synced)
    {
        Serial.println("Failed to obtain NTP time");
        return synced;
    }

    Serial.println("NTP time synchronized");
    return synced;
}

unsigned long TimeClass::getTimeFromNTP(Connection &connection)
{
    // Client &tcpClient = connection.getClient();
    const char *ntpServer = "time.nist.gov";
    const uint16_t ntpPort = 37; // NTP over TCP uses port 37
    const int ntpPacketSize = 4; // NTP time is 4 bytes
    Client &cli = connection.getClient();
    // Connect to NTP server
    if (!cli.connect(ntpServer, ntpPort))
    {
        Serial.println("Connection to NTP server failed.");
        return 0;
    }
    // Wait for response
    unsigned long startTime = millis();
    while (!cli.available() && (millis() - startTime) < 5000)
    {
        delay(10);
    }

    if (!cli.available())
    {
        Serial.println("No response from NTP server.");
        cli.stop();
        return 0;
    }
    // Read 4-byte NTP time
    byte buffer[ntpPacketSize];
    cli.read(buffer, ntpPacketSize);
    cli.stop();

    // Convert to Unix time
    unsigned long ntpTime = (unsigned long)buffer[0] << 24 |
                            (unsigned long)buffer[1] << 16 |
                            (unsigned long)buffer[2] << 8 |
                            (unsigned long)buffer[3];

    // NTP time starts on 1900-01-01, Unix time starts on 1970-01-01
    const unsigned long seventyYears = 2208988800UL;
    unsigned long unixTime = ntpTime - seventyYears;

    return unixTime;
}

bool TimeClass::parseMessage(const char *message)
{
    JsonDocument doc;
    deserializeJson(doc, message);
    unsigned long unixTime = doc["unix"]; // @todo:: correct
    if (unixTime > 0)
    {
        Serial.printf("Time parsed successfully, setting using MQTT to: %lu\n", unixTime);
        setSystemTime(unixTime); // Add one hour to compensate for UTC
        synced = true;
        return synced;
    }
    return false;
}
/**
 * @brief Get the time from a http server and return it as unix timestamp
 *
 * @param connection
 * @return unsigned long
 */
unsigned long TimeClass::getTimeFromHttp(Connection &connection)
{
    // ESP_SSLClient sslClient;

    // sslClient.setClient(connection.getClient());
    SecureClient &cli = connection.secureClient();
    SSLClientESP32 ssl_client(&connection.secureClient());
    ssl_client.setCACertBundle(ca_cert_bundle_start);
    cli.setCACertBundle(ca_cert_bundle_start);
    cli.setInsecure();
    HttpClient httpClient(cli, serverAddress, 443);
    httpClient.get(resource);
    // Wait for the response
    httpClient.setTimeout(1000);
    int statusCode = httpClient.responseStatusCode();
    // sslClient.stop();
    if (statusCode != 200)
    {
        Log.warning(F("HTTP GET failed with status code: %d" CR), statusCode);
        cli.stop();
        return 0;
    }
    cli.stop();

    // Read the response body
    String response = httpClient.responseBody();
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, response);
    if (error)
    {
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.f_str());
        return 0;
    }

    // Extract the datetime string
    const char *datetime = doc["datetime"];
    Serial.printf("BRO! SHOW BME THIS DATE: %s \n", datetime);
    if (datetime == nullptr)
    {
        Serial.println(F("Failed to extract 'datetime' from JSON"));
        return 0;
    }

    // Parse the datetime string to a struct tm
    struct tm tm;
    if (strptime(datetime, "%Y-%m-%dT%H:%M:%S", &tm) == nullptr)
    {
        Serial.println(F("Failed to parse datetime string"));
        return 0;
    }

    // Convert to time_t and return as unsigned long
    time_t t = mktime(&tm);
    return static_cast<unsigned long>(t);
}
