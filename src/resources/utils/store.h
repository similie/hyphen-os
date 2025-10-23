#ifndef _PAYLOAD_STORE_H
#define _PAYLOAD_STORE_H
#include <Hyphen.h>
// #include <vector>
class PayloadStore
{
private:
    const uint8_t MAX_PAYLOADS = 10;
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
    bool push(String, String);
    String sanitize(const String &in)
    {
        String s = in;
        s.replace("\r", "");
        s.replace("\n", "");
        return s;
    };

    uint8_t popOneOffline();
    uint32_t countEntries();
    String *pop(uint8_t);
    uint8_t popOfflineCollection();
};

#endif