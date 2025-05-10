#include "system/sd-card.h"

// static mutex instance
SemaphoreHandle_t SDCard::spiMutex = nullptr;

SDCard::SDCard()
{
    // create the mutex once
    if (!spiMutex)
    {
        spiMutex = xSemaphoreCreateMutex();
    }
    pinMode(cdPin, INPUT_PULLUP);
}

SDCard::~SDCard()
{
    // no cleanup needed
}

bool SDCard::guardedBegin()
{
    // lock SPI bus
    xSemaphoreTake(spiMutex, portMAX_DELAY);
    // ensure SPI peripheral is up
    SPI.begin();
    // initialize at 18 MHz
    bool ok = sd.begin(chipSelect, SD_SCK_MHZ(18));
    xSemaphoreGive(spiMutex);
    return ok;
}

bool SDCard::init()
{
    initialized = false;
    if (!sdCardPresent())
    {
        return false;
    }
    initialized = guardedBegin();
    if (!initialized)
    {
        Serial.println("SD card initialization failed!");
    }
    return initialized;
}

bool SDCard::ready()
{
    // card present && (already init OR init now)
    return sdCardPresent() && (initialized || init());
}

bool SDCard::sdCardPresent()
{
    return digitalRead(cdPin) == LOW;
}

void SDCard::bridgeSpi()
{
    // quick SPI wake-up
    xSemaphoreTake(spiMutex, portMAX_DELAY);
    SPI.begin();
    SPI.beginTransaction(SPISettings(4000000, MSBFIRST, SPI_MODE0));
    SPI.endTransaction();
    xSemaphoreGive(spiMutex);
}

String SDCard::read(String path, unsigned long &startPoint, char term)
{
    String result;
    if (!ready())
        return result;

    xSemaphoreTake(spiMutex, portMAX_DELAY);
    SdFile file;
    if (!file.open(path.c_str(), O_READ) || !file.seekSet(startPoint))
    {
        initialized = false;
        file.close();
        xSemaphoreGive(spiMutex);
        return result;
    }

    int c;
    while ((c = file.read()) >= 0)
    {
        startPoint++;
        if (c == term)
            break;
        result += char(c);
    }
    file.close();
    xSemaphoreGive(spiMutex);
    return result;
}

void SDCard::overwrite(const char *path, const char *newContent)
{
    if (!ready())
        return;

    xSemaphoreTake(spiMutex, portMAX_DELAY);
    SdFile file;
    if (!file.open(path, O_WRONLY | O_CREAT | O_TRUNC))
    {
        initialized = false;
        Serial.println("Failed to open file for overwriting");
        xSemaphoreGive(spiMutex);
        return;
    }
    file.print(newContent);
    file.close();
    xSemaphoreGive(spiMutex);
}

uint32_t SDCard::append(String path, String message, bool appendNewLine)
{
    if (!ready())
        return 0;

    xSemaphoreTake(spiMutex, portMAX_DELAY);
    SdFile file;
    if (!file.open(path.c_str(), O_RDWR | O_CREAT | O_AT_END))
    {
        initialized = false;
        Serial.println("Failed to open file for writing");
        xSemaphoreGive(spiMutex);
        return 0;
    }
    size_t written = appendNewLine ? file.println(message) : file.print(message);
    uint32_t size = file.fileSize();
    file.close();
    xSemaphoreGive(spiMutex);
    return size;
}

uint32_t SDCard::append(String path, String message)
{
    return append(path, message, true);
}

uint32_t SDCard::appendln(String path, String message)
{
    return append(path, message, true);
}

// Background append task

static void appendlnTask(void *parameter)
{
    // parameters: [0]=path.c_str(), [1]=message.c_str()
    const char **p = static_cast<const char **>(parameter);
    SDCard sd;
    sd.init();
    sd.appendln(p[0], p[1]);
    delete[] p;
    vTaskDelete(NULL);
}

void SDCard::startAppendlnTask(String path, String message)
{
    // allocate C-strings array
    const char **params = new const char *[2];
    params[0] = strdup(path.c_str());
    params[1] = strdup(message.c_str());
    xTaskCreatePinnedToCore(
        appendlnTask,
        "AppendlnTask",
        4096,
        params,
        tskIDLE_PRIORITY + 1,
        nullptr,
        1 // run on core 1
    );
}