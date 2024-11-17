#include "resources/utils/sdi-12.h"

#ifndef all_weather_h
#define all_weather_h
#define ALL_WEATHER_PARAMS_TOTAL 17

enum
{
    // all weather
    solar = 0,
    precipitation = 1,
    strikes = 2,
    strike_distance = 3,
    wind_speed = 4,
    wind_direction = 5,
    gust_wind_speed = 6,
    air_temperature = 7,
    vapor_pressure = 8,
    atmospheric_pressure = 9,
    relative_humidity = 10,
    humidity_sensor_temperature = 11,
    x_orientation = 12,
    y_orientation = 13,
    null_val = 14,
    wind_speed_north = 15,
    wind_speed_east = 16,
};

class AllWeatherElements : public SDIParamElements
{
private:
    String valueMap[ALL_WEATHER_PARAMS_TOTAL] =
        {
            "sol",
            "pre",
            "s",
            "s_d",
            "ws",
            "wd",
            "gws",
            "t",
            "a_p",
            "p",
            "h",
            "hst",
            "x",
            "y",
            "null",
            "wsn",
            "wse"};

public:
    String getDeviceName()
    {
        return "AllWeather";
    }

    size_t getBuffSize()
    {

        return 250;
    }

    uint8_t getTotalSize()
    {
        return (uint8_t)ALL_WEATHER_PARAMS_TOTAL;
    }
    String *getValueMap()
    {
        return valueMap;
    }

    size_t nullValue()
    {
        return null_val;
    }

    float extractValue(float values[], size_t key, size_t max)
    {
        switch (key)
        {
        case gust_wind_speed:
        case strike_distance:
            return utils.getMax(values, max);
        case precipitation:
        case strikes:
            return utils.getSum(values, max);
        default:
            return utils.getMedian(values, max);
        }
    }
};

class AllWeather : public Device
{
private:
    AllWeatherElements elements;
    SDI12Device *sdi;

public:
    ~AllWeather();
    AllWeather(Bootstrap *boots);
    AllWeather(Bootstrap *boots, int identity);

    void read();
    void loop();
    void clear();
    void print();
    void init();
    String name();
    uint8_t maintenanceCount();
    uint8_t paramCount();
    size_t buffSize();
    void publish(JsonObject &writer, uint8_t attempt_count);
    void restoreDefaults();
};

#endif