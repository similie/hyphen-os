
#ifndef hyphen_h
#define hyphen_h

#include "Arduino.h"
#include "string.h"
#include <EEPROM.h>
#include <iostream>
#include <string>
#include <map>
#include <functional>
#include "timer.h"
#include "system/modules.h"
#include "HyphenConnect.h"

class HyphenClass
{
private: // Map to store function names and corresponding std::function objects
    const String thisDeviceId = String(DEVICE_PUBLIC_ID);
    std::map<std::string, std::function<int(const std::string &)>>
        functions;
    HyphenConnect hyphen;
    String timeConfigTopic = String(MQTT_TOPIC_BASE) + "Config/" + thisDeviceId + "/Time";
    String timeConfigRequestTopic = String(MQTT_TOPIC_BASE) + "Config/Time";

public:
    HyphenClass();
    bool keepAlive(uint8_t);
    bool syncTime();
    bool hasHeartbeat();
    bool ready();
    void variable(const char *, bool *);
    void variable(const char *, int *);
    void variable(const char *, unsigned int *);
    void variable(const char *, long *);
    void variable(const char *, unsigned long *);
    void variable(const char *, double *);
    void variable(const char *, String *);

    void variable(String, bool *);
    void variable(String, int *);
    void variable(String, unsigned int *);
    void variable(String, long *);
    void variable(String, unsigned long *);
    void variable(String, double *);
    void variable(String, String *);

    // Non-template overloads
    void function(const char *name, std::function<int(const std::string &)> func);
    void function(const std::string, std::function<int(const std::string &)> func);
    void function(String, std::function<int(const std::string &)> func);

    void function(const char *name, std::function<int(const std::string &)> func, void *instance);

    // Template function overloads (fully defined here)
    template <typename T>
    void function(const char *name, int (T::*func)(String), T *instance);

    template <typename T>
    void function(const char *name, int (T::*func)(const std::string &), T *instance);

    template <typename T>
    void function(const std::string, int (T::*func)(const std::string &), T *instance);

    template <typename T>
    void function(String, int (T::*func)(const std::string &), T *instance);

    template <typename T>
    void function(String, int (T::*func)(String), T *instance);

    const String deviceID();
    bool connected();
    void disconnect();
    bool connect();
    void process();
    void reset();
    bool publish(String, String);
    HyphenConnect &hyConnect();
    void requestTime();
    void setSubscriptions();
    GPSData getLocation();
    bool connectionOn();
    bool connectionOff();
    bool isOnline();
    // void requestConfig();
};

extern HyphenClass Hyphen;

template <typename T>
void HyphenClass::function(const char *name, int (T::*func)(String), T *instance)
{
    std::function<int(const std::string &)> boundFunc = [func, instance](const std::string &param)
    {
        return (instance->*func)(String(param.c_str()));
    };
    function(name, boundFunc);
}

template <typename T>
void HyphenClass::function(const char *name, int (T::*func)(const std::string &), T *instance)
{
    std::function<int(const std::string &)> boundFunc = [func, instance](const std::string &param)
    {
        return (instance->*func)(param);
    };
    function(name, boundFunc);
}

template <typename T>
void HyphenClass::function(const std::string name, int (T::*func)(const std::string &), T *instance)
{
    return function(name.c_str(), func, instance);
}

template <typename T>
void HyphenClass::function(String name, int (T::*func)(const std::string &), T *instance)
{
    return function(name.c_str(), func, instance);
}

template <typename T>
void HyphenClass::function(String name, int (T::*func)(String), T *instance)
{
    return function(name.c_str(), func, instance);
}

#endif
