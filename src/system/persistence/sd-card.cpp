// SDCard.cpp
#include "system/sd-card.h"

SemaphoreHandle_t SDCard::spiMutex = nullptr;
SDCard *g_sdcardInstance = nullptr; // For ISR to reference

SDCard::SDCard()
    : _cardPresent(false),
      _lastDebounce(0),
      _pendingEvent(false)
{
    // Create SPI mutex once
    if (!spiMutex)
    {
        spiMutex = xSemaphoreCreateMutex();
    }

    pinMode(cdPin, INPUT_PULLUP);
    // Record initial state
    _cardPresent = (digitalRead(cdPin) == LOW);

    // Register this instance globally so ISR can call it
    g_sdcardInstance = this;
}

SDCard::~SDCard()
{
    // Nothing to free
}

bool SDCard::exists(const String &path)
{
    if (!init())
        return false;

    xSemaphoreTake(spiMutex, portMAX_DELAY);
    bool ok = sd.exists(path.c_str());
    xSemaphoreGive(spiMutex);
    return ok;
}

uint64_t SDCard::fileSize(const String &path)
{
    if (!init())
        return 0;

    xSemaphoreTake(spiMutex, portMAX_DELAY);

    SdFile file;
    uint64_t size = 0;

    if (file.open(path.c_str(), O_READ))
    {
        size = file.fileSize(); // <-- SdFat gives us the real size
        file.close();
    }

    xSemaphoreGive(spiMutex);
    return size;
}

bool SDCard::sdCardPresent() const
{
    // If the card-detect switch is active-low:
    return (digitalRead(cdPin) == LOW);
}

bool SDCard::guardedBegin()
{
    xSemaphoreTake(spiMutex, portMAX_DELAY);
    SPI.begin(); // Ensure SPI is powered up
    bool ok = sd.begin(chipSelect, SD_SCK_MHZ(25));
    xSemaphoreGive(spiMutex);
    return ok;
}

bool SDCard::init()
{
    // Called when you want to ensure SD.begin() is called once the card is present
    if (initialized)
        return true;

    if (!sdCardPresent())
        return false;

    if (!guardedBegin())
    {
        initialized = false;
        return false;
    }

    initialized = true;
    return true;
}

bool SDCard::ready()
{
    // True if card is inserted AND we have successfully called init()
    return sdCardPresent() && initialized;
}

uint32_t SDCard::countLines(const String &path, unsigned long startPos)
{
    if (!init())
        return 0;

    xSemaphoreTake(spiMutex, portMAX_DELAY);

    SdFile file;
    uint32_t count = 0;

    if (file.open(path.c_str(), O_READ))
    {
        if (!file.seekSet(startPos))
        {
            Serial.println("[SDCard] seekSet() failed!");
            file.close();
            xSemaphoreGive(spiMutex);
            return 0;
        }
        const size_t BUF_SIZE = 512;
        uint8_t buf[BUF_SIZE];
        int n;

        while ((n = file.read(buf, BUF_SIZE)) > 0)
        {
            for (int i = 0; i < n; i++)
            {
                if (buf[i] == '\n')
                    count++;
            }
        }
        file.close();
    }

    xSemaphoreGive(spiMutex);
    return count;
}

String SDCard::read(const String &path, unsigned long &startPoint, char terminatingChar)
{
    String result;
    if (!init())
        return result;

    xSemaphoreTake(spiMutex, portMAX_DELAY);
    SdFile file;
    if (file.open(path.c_str(), O_READ) && file.seekSet(startPoint))
    {
        int c;
        while ((c = file.read()) >= 0)
        {
            startPoint++;
            if (c == terminatingChar)
                break;
            result += char(c);
        }
        file.close();
    }
    xSemaphoreGive(spiMutex);
    return result;
}

bool SDCard::overwrite(const char *path, const char *newContent)
{
    if (!init())
        return false;

    xSemaphoreTake(spiMutex, portMAX_DELAY);
    SdFile file;
    bool ok = file.open(path, O_WRONLY | O_CREAT | O_TRUNC);
    if (ok)
    {
        file.print(newContent);
        file.close();
    }
    xSemaphoreGive(spiMutex);
    return ok;
}

uint64_t SDCard::appendln(const String &path, const String &message)
{
    if (!init())
        return 0;

    xSemaphoreTake(spiMutex, portMAX_DELAY);
    SdFile file;
    uint64_t size = 0;
    if (file.open(path.c_str(), O_RDWR | O_CREAT | O_AT_END))
    {
        file.println(message);
        size = file.fileSize();
        file.close();
    }
    xSemaphoreGive(spiMutex);
    return size;
}

void SDCard::bridgeSpi()
{
    if (!spiMutex)
        return;
    xSemaphoreTake(spiMutex, portMAX_DELAY);
    SPI.begin();
    SPI.beginTransaction(SPISettings(4000000, MSBFIRST, SPI_MODE0));
    SPI.endTransaction();
    xSemaphoreGive(spiMutex);
}

// -------------------------------------------------------------
//  Card Detect ISR & Debounce Logic
// -------------------------------------------------------------

void IRAM_ATTR SDCard::cardDetectISR()
{
    // Very short work in ISR: read new pin state and set flag if changed
    uint32_t now = xTaskGetTickCountFromISR();
    bool current = (digitalRead(g_sdcardInstance->cdPin) == LOW);

    // Simple debounce: ignore changes within 50 ms
    if ((now - g_sdcardInstance->_lastDebounce) > (50 / portTICK_PERIOD_MS))
    {
        g_sdcardInstance->_lastDebounce = now;
        g_sdcardInstance->_cardPresent = current;
        g_sdcardInstance->_pendingEvent = true;
    }
    // else ignore bounce
}

void SDCard::beginCardDetectInterrupt()
{
    // Attach rising and falling edge on cdPin
    // When card inserted, cdPin goes LOW; when removed, goes HIGH (assuming INPUT_PULLUP wiring)
    attachInterrupt(digitalPinToInterrupt(cdPin), SDCard::cardDetectISR, CHANGE);
}

// Called periodically in non-ISR context to handle the event
void SDCard::handleCardDetectEvent()
{
    if (!_pendingEvent)
        return;

    // Clear the flag
    _pendingEvent = false;

    if (_cardPresent)
    {
        // Card was just inserted
        Serial.println("SD: Card Inserted → initializing...");
        if (init())
        {
            Serial.println("SD: Initialization succeeded");
        }
        else
        {
            Serial.println("SD: Initialization failed");
        }
    }
    else
    {
        // Card was just removed
        Serial.println("SD: Card Removed → deinitializing...");
        // If you want to clean up, close any open files:
        // sd.closeAll();
        initialized = false;
    }
}
// // Static members
// SemaphoreHandle_t SDCard::spiMutex = nullptr;
// QueueHandle_t SDCard::jobQueue = nullptr;

// SDCard::SDCard()
// {
//     // Create SPI mutex once
//     if (!spiMutex)
//     {
//         spiMutex = xSemaphoreCreateMutex();
//     }
//     // Create job queue and worker task once
//     if (!jobQueue)
//     {
//         jobQueue = xQueueCreate(JOB_QUEUE_LENGTH, sizeof(SDJob));
//         // xTaskCreatePinnedToCore(
//         //     workerTask, "SDWorker", 4096, nullptr, tskIDLE_PRIORITY + 1, nullptr, 1);
//     }
//     pinMode(cdPin, INPUT_PULLUP);
// }

// SDCard::~SDCard()
// {
//     // Nothing to clean up (tasks/queues live for app lifetime)
// }

// bool SDCard::guardedBegin()
// {
//     xSemaphoreTake(spiMutex, portMAX_DELAY);
//     SPI.begin();
//     bool ok = sd.begin(chipSelect, SD_SCK_MHZ(18));
//     xSemaphoreGive(spiMutex);
//     return ok;
// }

// bool SDCard::sdCardPresent()
// {
//     return digitalRead(cdPin) == LOW;
// }

// bool SDCard::init()
// {
//     if (initialized)
//         return true;
//     if (!sdCardPresent())
//         return false;
//     initialized = guardedBegin();
//     return initialized;
// }

// bool SDCard::ready() const
// {
//     return digitalRead(cdPin) == LOW && initialized;
// }

// String SDCard::read(const String &path, unsigned long &startPoint, char terminatingChar)
// {
//     String result;
//     if (!init())
//         return result;
//     xSemaphoreTake(spiMutex, portMAX_DELAY);
//     SdFile file;
//     if (file.open(path.c_str(), O_READ) && file.seekSet(startPoint))
//     {
//         int c;
//         while ((c = file.read()) >= 0)
//         {
//             startPoint++;
//             if (c == terminatingChar)
//                 break;
//             result += char(c);
//         }
//         file.close();
//     }
//     xSemaphoreGive(spiMutex);
//     return result;
// }

// bool SDCard::overwrite(const char *path, const char *newContent)
// {
//     if (!init())
//         return false;
//     xSemaphoreTake(spiMutex, portMAX_DELAY);
//     SdFile file;
//     bool ok = file.open(path, O_WRONLY | O_CREAT | O_TRUNC);
//     if (ok)
//     {
//         file.print(newContent);
//         file.close();
//     }
//     xSemaphoreGive(spiMutex);
//     return ok;
// }

// uint64_t SDCard::appendln(const String &path, const String &message)
// {
//     if (!init())
//         return 0;
//     xSemaphoreTake(spiMutex, portMAX_DELAY);
//     SdFile file;
//     uint64_t size = 0;
//     if (file.open(path.c_str(), O_RDWR | O_CREAT | O_AT_END))
//     {
//         file.println(message);
//         size = file.fileSize();
//         file.close();
//     }
//     xSemaphoreGive(spiMutex);
//     return size;
// }

// bool SDCard::appendlnAsync(const String &path, const String &message, uint64_t maxBytes)
// {
//     if (!init())
//         return false;
//     // Allocate job
//     SDJob job;
//     job.path = strdup(path.c_str());
//     job.message = strdup(message.c_str());
//     job.maxBytes = maxBytes;
//     // Enqueue (fail if full)
//     return xQueueSend(jobQueue, &job, 0) == pdTRUE;
// }

// void SDCard::bridgeSpi()
// {
//     if (!spiMutex)
//         return;
//     xSemaphoreTake(spiMutex, portMAX_DELAY);
//     SPI.begin();
//     SPI.beginTransaction(SPISettings(4'000'000, MSBFIRST, SPI_MODE0));
//     SPI.endTransaction();
//     xSemaphoreGive(spiMutex);
// }

// void SDCard::workerTask(void *pv)
// {
//     SDJob job;
//     SDCard sd; // local helper
//     while (true)
//     {
//         if (xQueueReceive(jobQueue, &job, portMAX_DELAY) == pdTRUE)
//         {
//             // Perform appendln
//             if (sd.init())
//             {
//                 uint32_t length = sd.appendln(job.path, job.message);
//                 if (job.maxBytes && length > job.maxBytes)
//                 {
//                     sd.overwrite(job.path, job.message); // or trim logic
//                 }
//             }
//             // Free job memory
//             free(job.path);
//             free(job.message);
//         }
//         // vTaskDelay(pdMS_TO_TICKS(10)); // Yield to other tasks
//         taskYIELD();
//     }
// }
// #include "system/sd-card.h"

// // static mutex instance
// SemaphoreHandle_t SDCard::spiMutex = nullptr;

// SDCard::SDCard()
// {
//     // create the mutex once
//     if (!spiMutex)
//     {
//         spiMutex = xSemaphoreCreateMutex();
//     }
//     pinMode(cdPin, INPUT_PULLUP);
// }

// SDCard::~SDCard()
// {
//     // no cleanup needed
// }

// bool SDCard::guardedBegin()
// {
//     // lock SPI bus
//     xSemaphoreTake(spiMutex, portMAX_DELAY);
//     // ensure SPI peripheral is up
//     SPI.begin();
//     // initialize at 18 MHz
//     bool ok = sd.begin(chipSelect, SD_SCK_MHZ(18));
//     xSemaphoreGive(spiMutex);
//     return ok;
// }

// bool SDCard::init()
// {
//     initialized = false;
//     if (!sdCardPresent())
//     {
//         return false;
//     }
//     initialized = guardedBegin();
//     if (!initialized)
//     {
//         Serial.println("SD card initialization failed!");
//     }
//     return initialized;
// }

// bool SDCard::ready()
// {
//     // card present && (already init OR init now)
//     return sdCardPresent() && (initialized || init());
// }

// bool SDCard::sdCardPresent()
// {
//     return digitalRead(cdPin) == LOW;
// }

// void SDCard::bridgeSpi()
// {
//     // quick SPI wake-up
//     xSemaphoreTake(spiMutex, portMAX_DELAY);
//     SPI.begin();
//     SPI.beginTransaction(SPISettings(4000000, MSBFIRST, SPI_MODE0));
//     SPI.endTransaction();
//     xSemaphoreGive(spiMutex);
// }

// String SDCard::read(String path, unsigned long &startPoint, char term)
// {
//     String result;
//     if (!ready())
//         return result;

//     xSemaphoreTake(spiMutex, portMAX_DELAY);
//     SdFile file;
//     if (!file.open(path.c_str(), O_READ) || !file.seekSet(startPoint))
//     {
//         initialized = false;
//         file.close();
//         xSemaphoreGive(spiMutex);
//         return result;
//     }

//     int c;
//     while ((c = file.read()) >= 0)
//     {
//         startPoint++;
//         if (c == term)
//             break;
//         result += char(c);
//     }
//     file.close();
//     xSemaphoreGive(spiMutex);
//     return result;
// }

// void SDCard::overwrite(const char *path, const char *newContent)
// {
//     if (!ready())
//         return;

//     xSemaphoreTake(spiMutex, portMAX_DELAY);
//     SdFile file;
//     if (!file.open(path, O_WRONLY | O_CREAT | O_TRUNC))
//     {
//         initialized = false;
//         Serial.println("Failed to open file for overwriting");
//         xSemaphoreGive(spiMutex);
//         return;
//     }
//     file.print(newContent);
//     file.close();
//     xSemaphoreGive(spiMutex);
// }

// uint32_t SDCard::append(String path, String message, bool appendNewLine)
// {
//     if (!ready())
//         return 0;

//     xSemaphoreTake(spiMutex, portMAX_DELAY);
//     SdFile file;
//     if (!file.open(path.c_str(), O_RDWR | O_CREAT | O_AT_END))
//     {
//         initialized = false;
//         Serial.println("Failed to open file for writing");
//         xSemaphoreGive(spiMutex);
//         return 0;
//     }
//     size_t written = appendNewLine ? file.println(message) : file.print(message);
//     uint32_t size = file.fileSize();
//     file.close();
//     xSemaphoreGive(spiMutex);
//     return size;
// }

// uint32_t SDCard::append(String path, String message)
// {
//     return append(path, message, true);
// }

// uint32_t SDCard::appendln(String path, String message)
// {
//     return append(path, message, true);
// }

// // Background append task

// void SDCard::appendlnTask(void *parameter)
// {

//     auto params = static_cast<char **>(parameter);
//     SDCard sd;
//     if (sd.init())
//     {
//         // Serial.printf("Appending to %s: %s\n", params[0], params[1]);
//         uint32_t length = sd.appendln(params[0], params[1]);
//         if (!String(params[2]).equals("0"))
//         {
//             uint32_t maxBytes = atoi(params[2]);
//             if (length > maxBytes)
//                 sd.overwrite(params[0], params[2]);
//         }
//     }
//     // clean up malloc’d memory
//     free(params[0]);
//     free(params[1]);
//     free(params[2]);
//     delete[] params;
//     vTaskDelete(NULL);
// }

// void SDCard::appendlnAsync(String path, String message, uint32_t maxBytes)
// {
//     // allocate C-strings array
//     const char **params = new const char *[3];
//     params[0] = strdup(path.c_str());
//     params[1] = strdup(message.c_str());
//     params[2] = strdup(maxBytes ? String(maxBytes).c_str() : "0");

//     xTaskCreatePinnedToCore(
//         SDCard::appendlnTask,
//         "AppendlnTask",
//         4096,
//         params,
//         tskIDLE_PRIORITY + 1,
//         nullptr,
//         1 // run on core 1
//     );
// }