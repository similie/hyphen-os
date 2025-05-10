#include "store.h"

PayloadStore::PayloadStore()
{
}

bool PayloadStore::push(String topic, String payload)
{
    if (!Storage.sdCardPresent())
    {
        return false;
    }
    uint32_t addPosition = Storage.appendln(storeFile, sanitize(topic + "|" + payload));
    return addPosition > 0;
}

void PayloadStore::resetStorageFile()
{
    if (!Storage.sdCardPresent())
    {
        return;
    }
    setPopPosition(0);
    Storage.overwrite(storeFile.c_str(), "");
}

String *PayloadStore::pop(uint8_t size)
{
    String *result = new String[size];
    unsigned long position = getPopPosition();

    Serial.println("Pop position: " + String(position));
    for (uint8_t i = 0; i < size; i++)
    {
        result[i] = "";
        if (!Storage.sdCardPresent())
        {
            continue;
        }

        String value = Storage.read(storeFile, position, '\n');
        // here we have an empty value, we can break the loop.
        if (position == 0 && value.isEmpty())
        {
            break;
            // we are at the end of file, we need to reset i
        }
        else if (value.isEmpty())
        {
            position = 0;
            resetStorageFile();
            break;
        }

        result[i] = value;
    }
    Serial.println("Pop result POSITION: " + String(position));
    if (position > 0)
    {
        setPopPosition(position);
    }

    return result;
}

unsigned long PayloadStore::getPopPosition()
{
    if (!Storage.sdCardPresent())
    {
        return 0;
    }

    unsigned long position = 0;
    Persist.get(popKey, position);
    return position;
    // String value = Storage.read(positionFile, position, '\n');

    // if (value.isEmpty())
    // {
    //     setPopPosition(0);
    //     return 0;
    // }
    // return value.toInt();
}

bool PayloadStore::setPopPosition(unsigned long position)
{
    if (!Storage.sdCardPresent())
    {
        return false;
    }

    Persist.put(popKey, position);
    // String value = String(position) + "\n";
    // Storage.overwrite(positionFile.c_str(), value.c_str());
    return true;
}

String PayloadStore::setStale(String payload)
{
    JsonDocument doc;
    deserializeJson(doc, payload);
    doc["stale"] = true;
    String newPayload;
    serializeJson(doc, newPayload);
    return newPayload;
}

void PayloadStore::addBackOntoStore(uint8_t startIndex, String *result, uint8_t size)
{

    for (uint8_t i = startIndex; i < size; i++)
    {
        if (result[i].isEmpty())
        {
            break;
        }
        Log.errorln("Adding back onto store: %s", result[i].c_str());
        uint32_t addPosition = Storage.appendln(storeFile, sanitize(result[i]));
        if (addPosition == 0)
        {
            break;
        }
    }
}

uint32_t PayloadStore::countEntries()
{
    // No SD, no entries
    if (!Storage.sdCardPresent())
    {
        return 0;
    }

    uint32_t count = 0;
    unsigned long pos = getPopPosition();
    // Read until we hit an empty record
    while (true)
    {
        // grab everything from pos up to the next '\n'
        String line = Storage.read(storeFile, pos, '\n');
        if (line.isEmpty())
        {
            // either EOF or blank line → stop
            break;
        }
        count++;
    }

    return count;
}

uint8_t PayloadStore::popOfflineCollection(uint8_t size, unsigned long delay)
{
    String *result = pop(size);
    uint8_t count = 0;
    for (uint8_t i = 0; i < size; i++)
    {
        coreDelay(delay);
        if (result[i].isEmpty())
        {
            break;
        }

        String topic = result[i].substring(0, result[i].indexOf("|"));
        String payload = result[i].substring(result[i].indexOf("|") + 1);
        String send = setStale(payload);
        if (send.isEmpty())
        {
            continue;
        }
        Serial.printf("Topic: %s \n", topic.c_str());
        Serial.println("Sending offline payload: " + send);
        if (Hyphen.publish(sanitize(topic), setStale(payload)))
        {
            Serial.println("Offline payload sent successfully");
            count++;
        }
        else
        {
            Serial.println("Failed to send offline payload");
            break;
        }
    }
    Serial.println("Count: " + String(count) + " / " + String(size));
    if (count < size)
    {
        addBackOntoStore(count, result, size);
    }

    delete[] result; // Deallocate the array
    return count;
}

uint8_t PayloadStore::popOneOffline()
{
    return popOfflineCollection(1, 10);
}

uint8_t PayloadStore::popOfflineCollection()
{
    return popOfflineCollection(MAX_PAYLOADS, 1000);
}