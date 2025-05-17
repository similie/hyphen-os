// SDCard.h
#ifndef _SD_CARD_H
#define _SD_CARD_H

#include <SPI.h>
#include <SdFat.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/queue.h>
#include <freertos/task.h>
#include <Arduino.h>

struct SDJob
{
    char *path;
    char *message;
    uint64_t maxBytes;
};

class SDCard
{
public:
    SDCard();
    ~SDCard();

    // Initialize card (must call once before anything else)
    bool init();

    // Is card ready?
    bool ready() const;

    // Read up to terminatingChar (default '\n') from path at startPoint
    String read(const String &path, unsigned long &startPoint, char terminatingChar = '\n');

    // Overwrite entire file
    bool overwrite(const char *path, const char *newContent);

    // Append line synchronously
    uint64_t appendln(const String &path, const String &message);

    // Append line asynchronously; returns false on queue full
    // bool appendlnAsync(const String &path, const String &message, uint64_t maxBytes = 0);

    // Quick SPI wake-up (optional)
    void bridgeSpi();

    bool sdCardPresent();

private:
    // Helper to lock SPI and call sd.begin()
    bool guardedBegin();

    // Worker task for async appends
    // static void workerTask(void *pv);

    SdFat sd;
    bool initialized = false;
    const uint8_t cdPin = 12;
    const uint8_t chipSelect = SS;

    // Shared mutex for SPI bus
    static SemaphoreHandle_t spiMutex;

    // Queue for SDJob jobs
    static QueueHandle_t jobQueue;
    static const int JOB_QUEUE_LENGTH = 10;
};

#endif // _SD_CARD_H
       // #ifndef _SD_CARD_H
       // #define _SD_CARD_H

// #include <SPI.h>
// #include <SdFat.h>
// #include <freertos/FreeRTOS.h>
// #include <freertos/semphr.h>

// class SDCard
// {
// private:
//     bool initialized = false;
//     SdFat sd;
//     const uint8_t cdPin = 12;      // Card Detect pin
//     const uint8_t chipSelect = SS; // CS pin

//     // One mutex for the entire SD/SPI bus:
//     static SemaphoreHandle_t spiMutex;
//     static void appendlnTask(void *parameter);
//     // Internal helper to lock, init SPI, then call sd.begin():
//     bool guardedBegin();

// public:
//     SDCard();
//     ~SDCard();

//     // Must be called once before using read/append/etc.
//     // Returns true if the card is present and initialized.
//     bool init();

//     // Returns true if card is present & init() has succeeded (or re-attempts it)
//     bool ready();

//     // Just checks the card detect pin
//     bool sdCardPresent();

//     // Read up to 'term' (default '\n') from 'path' starting at startPoint.
//     String read(String path, unsigned long &startPoint, char terminatingChar);

//     // Overwrite the entire file
//     void overwrite(const char *path, const char *newContent);

//     // Append (without newline) or appendln (with newline)
//     uint32_t append(String path, String message, bool appendNewLine = false);
//     uint32_t append(String path, String message);
//     uint32_t appendln(String path, String message);

//     // Launch a background task to appendln on core 1
//     void appendlnAsync(String path, String message, uint32_t maxBytes = 0);

//     // A tiny helper to ensure SPI bus is awake/configured before use
//     void bridgeSpi();
// };

// #endif // _SD_CARD_H