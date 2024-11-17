#ifndef PERSISTENCE_H
#define PERSISTENCE_H

#include <Preferences.h>

class Persistence
{
public:
    const String persistanceAddressBase = "address_key_";
    explicit Persistence(const char *namespaceName = "hyphen_storage");
    bool begin();
    void end();
    template <typename T>
    bool put(const char *key, const T &value);
    template <typename T>
    bool get(const char *key, T &value);
    template <typename T>
    bool put(uint16_t key, const T &value);
    template <typename T>
    bool get(uint16_t key, T &value);
    void clear();

private:
    bool isOpen = false;
    Preferences preferences;
    const char *namespaceName;
    String keyToString(uint16_t key);
    void openPreferences();
    void closePreferences();
};

template <typename T>
bool Persistence::put(const char *key, const T &value)
{
    openPreferences();
    size_t dataSize = sizeof(T);
    size_t writtenSize = preferences.putBytes(key, &value, dataSize);
    closePreferences();
    return (writtenSize == dataSize);
}

template <typename T>
bool Persistence::get(const char *key, T &value)
{
    openPreferences();
    size_t dataSize = sizeof(T);
    Serial.printf("get %s is key %d \n", key, preferences.isKey(key));
    if (!preferences.isKey(key))
    {
        return false; // Key not found
    }
    if (preferences.getBytesLength(key) != dataSize)
    {
        return false; // Size mismatch
    }
    size_t readSize = preferences.getBytes(key, &value, dataSize);
    Serial.printf("get length  %d is not %d and not %d \n", preferences.getBytesLength(key), dataSize, readSize);
    closePreferences();
    return (readSize == dataSize);
}

template <typename T>
bool Persistence::put(uint16_t key, const T &value)
{
    String keyStr = persistanceAddressBase + keyToString(key);
    Serial.printf("Putting %s\n", keyStr.c_str());
    return put(keyStr.c_str(), value);
}

template <typename T>
bool Persistence::get(uint16_t key, T &value)
{
    String keyStr = persistanceAddressBase + keyToString(key);
    Serial.printf("Getting %s\n", keyStr.c_str());
    return get(keyStr.c_str(), value);
}

#endif // PERSISTENCE_H