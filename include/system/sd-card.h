// SDCardWithDetect.h

#ifndef _SD_CARD_WITH_DETECT_H
#define _SD_CARD_WITH_DETECT_H

#include <SPI.h>
#include <SdFat.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
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

    // Initialize card (must call this once)
    bool init();

    // Returns true if card is present & initialized
    bool ready();

    // Read up to 'term' (default '\n') from 'path' starting at startPoint.
    String read(const String &path, unsigned long &startPoint, char terminatingChar = '\n');

    // Overwrite entire file
    bool overwrite(const char *path, const char *newContent);

    // Append line synchronously
    uint64_t appendln(const String &path, const String &message);

    // Quick SPI wake-up (optional)
    void bridgeSpi();

    // Returns true if card detect pin indicates presence
    bool sdCardPresent() const;

    uint32_t countLines(const String &path, unsigned long startPos = 0);

    // Call in your setup() to install the ISR
    void beginCardDetectInterrupt();

    uint64_t fileSize(const String &path);
    bool exists(const String &path);

private:
    // Internal helper to lock SPI and call sd.begin()
    bool guardedBegin();

    // Called from ISR
    static void IRAM_ATTR cardDetectISR();

    // To be run in main code context whenever card state changed
    void handleCardDetectEvent();

    // Shared mutex for SPI bus
    static SemaphoreHandle_t spiMutex;

    // Card-detect pin
    const uint8_t cdPin = 12;        // Card Detect pin
    volatile bool _cardPresent;      // true if card is inserted
    volatile uint32_t _lastDebounce; // timestamp of last change
    volatile bool _pendingEvent;     // flag set by ISR

    SdFat sd;
    bool initialized = false;
    const uint8_t chipSelect = SS;

    // Disable copy
    SDCard(const SDCard &) = delete;
    SDCard &operator=(const SDCard &) = delete;
};

#endif