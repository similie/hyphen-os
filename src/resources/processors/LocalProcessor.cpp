#include "LocalProcessor.h"

/**
 *
 * The LocalProcessor class can inherited for producing additional means of sending data payloads such as MQTT.
 * This default class uses particle's api for sending data. Future classes will use other protocols
 *
 */

/**
 * @deconstructor
 */
LocalProcessor::~LocalProcessor()
{
}

/**
 * Default Constructor
 */
LocalProcessor::LocalProcessor()
{
    aliveTimestamp = millis();
}

/**
 * @public
 *
 * getHeartbeatTopic
 *
 * What's the heartbeat topic name we send
 *
 * @return String
 */
String LocalProcessor::getHeartbeatTopic()
{
    return String(SEND_EVENT_HEARTBEAT);
}

/**
 * @public
 *
 * getPublishTopic
 *
 * What's the publish topic name we send
 *
 * @return String
 */
String LocalProcessor::getPublishTopic(bool maintenance)
{
    if (maintenance)
    {
        return String(this->SEND_EVENT_MAINTENANCE);
    }

    return String(this->SEND_EVENT_NAME);
}

/**
 * @public
 *
 * publish
 *
 * Sends the payload over the network
 *
 * @param topic - the topic name
 * @param payload - the actual stringified data
 *
 * @return bool - true if successful
 */
bool LocalProcessor::publish(const char *topic, const char *payload)
{
    bool success = Hyphen.publish(topic, payload);
    return success;
}

bool LocalProcessor::ready()
{
    return Hyphen.ready();
}

bool LocalProcessor::publish(String topic, String payload)
{
    return publish(topic.c_str(), payload.c_str());
}

bool LocalProcessor::publish(const char *topic, String payload)
{
    return publish(topic, payload.c_str());
}

// /**
//  * @public
//  *
//  * connected
//  *
//  * Is the device connected to the network it sends to
//  *
//  * @return bool - true if connected
//  */
// bool LocalProcessor::connected()
// {
//     return Hyphen.connected();
// }

/**
 * @public
 *
 * isConnected
 *
 * Is the device connected to the network it sends to.
 * Same as above. For API compatibility
 *
 * @return bool - true if connected
 */
bool LocalProcessor::isConnected()
{
    return Hyphen.connected();
}

/**
 * @public
 *
 * parseMessage
 *
 * Parses a given message based on a message topic
 *
 * @param String data - the message string
 * @param char *topic - the topic that was sent
 *
 * @return void
 */
void LocalProcessor::parseMessage(String data, char *topic)
{
    // JSONValue outerObj = JSONValue::parseCopy(data.c_str());
    // JSONObjectIterator iter(outerObj);
    // while (iter.next())
    // {
    //     Log.info("key=%s value=%s",
    //              (const char *)iter.name(),
    //              (const char *)iter.value().toString());
    // }
}

/**
 * @public
 *
 * loop
 *
 * Loops with the main loop. Some LocalProcessors require
 * a constant checkin, including particle in manual mode
 *
 * @return void
 */
void LocalProcessor::loop()
{
    // #ifndef HYPHEN_THREADED
    Hyphen.process();
    // #endif
}

/**
 * @public
 *
 * hasHeartbeat
 *
 * Is the heartbeat event ready
 *
 * @return bool
 */
bool LocalProcessor::hasHeartbeat()
{
    return this->HAS_HEARTBEAT;
}

/**
 * @public
 *
 * primaryPostName
 *
 * Sends the the primary send event name
 *
 * @return bool
 */
String LocalProcessor::primaryPostName()
{
    return this->SEND_EVENT_NAME;
}

/**
 * @public
 *
 * connect
 *
 * Connect to the send network
 *
 * @return void
 */
bool LocalProcessor::connect()
{
    return Hyphen.connect();
}

bool LocalProcessor::subscribe(const char *topic, std::function<void(const char *, const char *)> callback)
{
    return Hyphen.hyConnect().subscribe(topic, callback);
}

bool LocalProcessor::unsubscribe(const char *topic)
{
    return Hyphen.hyConnect().unsubscribe(topic);
}

bool LocalProcessor::init()
{
    size_t count = 0;

#ifdef HYPHEN_THREADED
    // 1) enqueue the setup exactly once
    if (!connect())
    {
        Utils::log("Failed to enqueue threaded connect", "MQTT");
        return false;
    }

    Utils::log("Threaded connection established", "MQTT");
    return true;

#else
    // single-threaded fallback
    while (!connect())
    {
        Utils::log("Connecting…", "MQTT");
        vTaskDelay(pdMS_TO_TICKS(5000));
        if (++count > 5)
        {
            Utils::log("Connect timed out", "MQTT");
            return false;
        }
    }
    Utils::log("Connection established", "MQTT");
    return true;
#endif
}

void LocalProcessor::disconnect()
{
    return Hyphen.disconnect();
}