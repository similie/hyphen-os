#ifndef _SD_CARD_H
#define _SD_CARD_H

#include <SPI.h>
#include <SdFat.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

class SDCard
{
private:
    bool initialized = false;
    SdFat sd;
    const uint8_t cdPin = 12;      // Card Detect pin
    const uint8_t chipSelect = SS; // CS pin

    // One mutex for the entire SD/SPI bus:
    static SemaphoreHandle_t spiMutex;

    // Internal helper to lock, init SPI, then call sd.begin():
    bool guardedBegin();

public:
    SDCard();
    ~SDCard();

    // Must be called once before using read/append/etc.
    // Returns true if the card is present and initialized.
    bool init();

    // Returns true if card is present & init() has succeeded (or re-attempts it)
    bool ready();

    // Just checks the card detect pin
    bool sdCardPresent();

    // Read up to 'term' (default '\n') from 'path' starting at startPoint.
    String read(String path, unsigned long &startPoint, char terminatingChar);

    // Overwrite the entire file
    void overwrite(const char *path, const char *newContent);

    // Append (without newline) or appendln (with newline)
    uint32_t append(String path, String message, bool appendNewLine = false);
    uint32_t append(String path, String message);
    uint32_t appendln(String path, String message);

    // Launch a background task to appendln on core 1
    void startAppendlnTask(String path, String message);

    // A tiny helper to ensure SPI bus is awake/configured before use
    void bridgeSpi();
};

#endif // _SD_CARD_H