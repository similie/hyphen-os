#include "Hyphen.h"
#include "string.h"
#include "resources/utils/utils.h"
#ifndef local_processor_h
#define local_processor_h

class LocalProcessor : public Processor
{
private:
    const bool HAS_HEARTBEAT = true;
#ifdef COMPRESSED_PUBLISH
    const char *SEND_EVENT_NAME = "Hy/Post/Gold";
#else
    const char *SEND_EVENT_NAME = "Hy/Post/Black";
#endif
    const char *SEND_EVENT_MAINTENANCE = "Hy/Post/Maintain";
    const char *SEND_EVENT_HEARTBEAT = "Hy/Post/Heartbeat";
    void manageManualModel();
    const bool MANUAL_MODE = false;

public:
    ~LocalProcessor();
    LocalProcessor();
    Utils utils;
    unsigned long aliveTimestamp = 0;
    const unsigned long PRINT_TIMEOUT = 60000;
    bool hasHeartbeat();
    String primaryPostName();
    static void parseMessage(String data, char *topic);
    String getHeartbeatTopic();
    String getPublishTopic(bool maintenance);
    bool connect();
    void disconnect();
    bool isConnected();
    bool init();
    bool ready();
    void loop();
    void maintain() {}
    bool publish(const char *topic, const char *payload);
    bool publish(String, String);
    bool publish(const char *topic, String);
    bool publish(const char *topic, uint8_t *buf, size_t size);
    bool unsubscribe(const char *);
    bool subscribe(const char *topic, std::function<void(const char *, const char *)> callback);
    bool compressPublish(String topic, String payload);
};

#endif