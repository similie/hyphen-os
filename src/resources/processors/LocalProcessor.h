#include "Hyphen.h"
#include "string.h"
#include "resources/utils/utils.h"
#ifndef local_processor_h
#define local_processor_h

class LocalProcessor : public Processor
{
private:
    const bool HAS_HEARTBEAT = true;
    /**
     * We send to different events to load balanance
     */
    // const char *SEND_EVENT_NAME = "Al/Post/Red";
    // const char *SEND_EVENT_NAME = "Al/Post/White";
    const char *SEND_EVENT_NAME = "Al/Post/Blue";
    // const char *SEND_EVENT_NAME = "Al/Post/Black";
    const char *SEND_EVENT_MAINTENANCE = "Al/Post/Maintain";
    const char *SEND_EVENT_HEARTBEAT = "Al/Post/Heartbeat";
    void manageManualModel();
    const bool MANUAL_MODE = false;

public:
    ~LocalProcessor();
    LocalProcessor();
    Utils utils;
    bool hasHeartbeat();
    String primaryPostName();
    static void parseMessage(String data, char *topic);
    String getHeartbeatTopic();
    String getPublishTopic(bool maintenance);
    bool connect();
    // bool connected(); // review
    void disconnect();
    bool isConnected();
    bool init();
    void loop();
    bool publish(const char *topic, const char *payload);
    bool publish(String, String);
    bool publish(const char *topic, String);
    bool subscribe(const char *topic, std::function<void(const char *, const char *)> callback);
};

#endif