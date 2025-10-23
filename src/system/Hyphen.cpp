#include "Hyphen.h"

HyphenClass::HyphenClass() : hyphen(ConnectionType::WIFI_ONLY)
{
}
void HyphenClass::process()
{

    if (unsubscribeTimeConfig)
    {
        unsubscribeTimeConfig = false;
        hyphen.unsubscribe(timeConfigTopic.c_str());
    }
    return hyphen.loop();
}
bool HyphenClass::keepAlive(uint8_t keepAlive)
{
    return true;
}
bool HyphenClass::syncTime()
{
    return Time.init(hyphen.getConnection());
}
bool HyphenClass::connectionOn()
{
    return hyphen.connectionOn();
}
bool HyphenClass::connectionOff()
{
    return hyphen.connectionOff();
}

bool HyphenClass::isOnline()
{
    return hyphen.isOnline();
}

bool HyphenClass::ready()
{
    return hyphen.ready();
}
void HyphenClass::requestTime()
{
    if (!connected())
    {
        Log.errorln("Not connected to the network for time request");
        return;
    }
    publish(timeConfigRequestTopic.c_str(), deviceID().c_str());
}

void HyphenClass::setSubscriptions()
{
    hyphen.subscribe(timeConfigTopic.c_str(), [this](const char *topic, const char *payload)
                     {
                        // if (Time.isSynced()) {
                        //     return;
                        // }
                         Log.infoln("Time config topic: %s", topic);
                         Log.infoln("Time config payload: %s", payload);
                         if(Time.parseMessage(payload)) {
                            unsubscribeTimeConfig = true;
                         } });
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
const String HyphenClass::deviceID()
{
    return thisDeviceId;
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
    ESP.restart();
}
bool HyphenClass::publish(String topic, String payload)
{
    return hyphen.publishTopic(topic, payload);
}

bool HyphenClass::publish(const char *topic, uint8_t *buf, size_t length)
{
    return hyphen.publishTopic(topic, buf, length);
}

bool HyphenClass::compressPublish(String topic, String payload)
{
    // mqttClient.publish_P
    const size_t JSON_LEN = payload.length();
    const size_t BUF_LEN = JSON_LEN + /*some extra*/ 16;
    JsonDocument doc;
    deserializeJson(doc, payload);
    uint8_t *buf = new uint8_t[BUF_LEN];
    size_t len = serializeMsgPack(doc.as<JsonVariantConst>(), buf, BUF_LEN);
    Log.noticeln("Publishing to topic: %s, %s, size: %d of %d", topic.c_str(), payload.c_str(), len, JSON_LEN);
    bool published = hyphen.publishTopic(topic.c_str(), buf, len);
    delete[] buf;
    return published;
}

HyphenConnect &HyphenClass::hyConnect()
{
    return hyphen;
}

GPSData HyphenClass::getLocation()
{
    return hyConnect().getLocation();
}

HyphenClass Hyphen;