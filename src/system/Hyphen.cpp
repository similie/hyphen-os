#include "Hyphen.h"

HyphenClass::HyphenClass() : hyphen(ConnectionType::WIFI_PREFERRED)
{
}

void HyphenClass::process()
{
    return hyphen.loop();
}

bool HyphenClass::keepAlive(uint8_t keepAlive)
{
    return true;
}
bool HyphenClass::syncTime()
{
    return Time.init();
}
void HyphenClass::variable(const char *name, int *variable)
{
    return hyphen.variable(name, variable);
}
void HyphenClass::variable(const char *name, double *variable)
{
    return hyphen.variable(name, variable);
}
void HyphenClass::variable(const char *name, String *variable)
{
    return hyphen.variable(name, variable);
}

void HyphenClass::variable(const char *name, unsigned int *variable)
{
    return hyphen.variable(name, (long *)variable);
}

void HyphenClass::variable(const char *name, long *variable)
{
    return hyphen.variable(name, variable);
}

void HyphenClass::variable(const char *name, unsigned long *variable)
{
    return hyphen.variable(name, (long *)variable);
}

void HyphenClass::variable(const char *name, bool *variable)
{
    return hyphen.variable(name, (int *)variable);
}

void HyphenClass::variable(String name, int *variable)
{
    return hyphen.variable(name.c_str(), variable);
}
/**
 * @brief Update the variable in the cloud
 *
 * @param name
 * @param variable
 */
void HyphenClass::variable(String name, unsigned int *variable)
{
    return hyphen.variable(name.c_str(), (long *)variable);
}

void HyphenClass::variable(String name, unsigned long *variable)
{
    return hyphen.variable(name.c_str(), (long *)variable);
}

void HyphenClass::variable(String name, double *variable)
{
    return hyphen.variable(name.c_str(), variable);
}
void HyphenClass::variable(String name, String *variable)
{
    return hyphen.variable(name.c_str(), variable);
}

void HyphenClass::variable(String name, long *variable)
{
    return hyphen.variable(name.c_str(), variable);
}

void HyphenClass::variable(String name, bool *variable)
{
    return hyphen.variable(name.c_str(), (int *)variable);
}

void HyphenClass::function(const char *name, std::function<int(const std::string &)> func)
{
    return hyphen.function(name, func);
}

void HyphenClass::function(const std::string name, std::function<int(const std::string &)> func)
{
    return hyphen.function(name.c_str(), func);
}

void HyphenClass::function(String name, std::function<int(const std::string &)> func)
{
    return hyphen.function(name.c_str(), func);
}

void HyphenClass::function(const char *name, std::function<int(const std::string &)> func, void *instance)
{
    hyphen.function(name, [func, instance](const char *params) -> int
                    {
        // Convert the params to a string and pass it to the func
        return func(std::string(params)); });
}
// void HyphenClass::function(const char *name, std::function<int(const std::string &)> *func, void *instance)
// {
// }
// template <typename T>
// void HyphenClass::function(const char *name, int (T::*func)(String), T *instance)
// {
//     std::function<int(const std::string &)> boundFunc = [func, instance](const std::string &param)
//     {
//         return (instance->*func)(String(param.c_str()));
//     };
//     function(name, boundFunc);
// }

// template <typename T>
// void HyphenClass::function(const char *name, int (T::*func)(const std::string &), T *instance)
// {
//     std::function<int(const std::string &)> boundFunc = [func, instance](const std::string &param)
//     {
//         return (instance->*func)(param);
//     };
//     function(name, boundFunc);
// }

//  void function(const char *, std::function<int(const std::string &)> *, void *);
//     template <typename T>
//     void function(const char *, int (T::*)(String), T *);

//     template <typename T>
//     void function(const char *, int (T::*)(const std::string &), T *);

String HyphenClass::deviceID()
{
    return String(DEVICE_PUBLIC_ID);
}
bool HyphenClass::connected()
{
    return hyphen.isConnected();
}
void HyphenClass::disconnect()
{
    return hyphen.disconnect();
}
bool HyphenClass::connect()
{
    return hyphen.setup(LOG_LEVEL_VERBOSE);
}
void HyphenClass::reset()
{
}
bool HyphenClass::publish(String topic, String payload)
{
    return hyphen.publishTopic(topic, payload);
}

HyphenConnect &HyphenClass::hyConnect()
{
    return hyphen;
}

HyphenClass Hyphen;