#ifndef _PAYLOAD_STORE_H
#define _PAYLOAD_STORE_H
#include <Hyphen.h>
// #include <vector>
#define LOG_FILE_NAME "hyphen-logs.txt"

class PayloadStore
{
private:
    static const uint64_t MAX_LOG_SIZE = 1000000;
    static const bool LOG_TO_FILE = true;
    String logFile = String(LOG_FILE_NAME);
    const uint8_t MAX_PAYLOADS = 10;
    std::vector<String> logBuffer;
    SemaphoreHandle_t logMutex = nullptr;
    TaskHandle_t flushTaskHandle = nullptr;
    static void flushTask(void *param);
    void flushToFile();

    const String positionFile = "position.txt";
    const String storeFile = "popStorage.txt";
    const char *popKey = "pop_key";
    void resetStorageFile();
    unsigned long getPopPosition();
    bool setPopPosition(unsigned long);
    String setStale(String);
    void addBackOntoStore(uint8_t, String *, uint8_t);
    uint8_t popOfflineCollection(uint8_t, unsigned long);

public:
    PayloadStore();
    void spawnFlushTask();
    bool push(String, String);
    String sanitize(const String &in)
    {
        String s = in;
        s.replace("\r", "");
        s.replace("\n", "");
        return s;
    };
    uint32_t log(String);
    uint8_t popOneOffline();
    uint32_t countEntries();
    String *pop(uint8_t);
    uint8_t popOfflineCollection();
};

#endif