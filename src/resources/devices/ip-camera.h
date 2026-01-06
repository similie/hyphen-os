#pragma once
#include "device.h"
#include "system/socket-config.h"
#include "resources/utils/wss-rtsp-tunnel.h"

// FreeRTOS
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>

struct IPCameraConfigStruct
{
    uint8_t version;
    uint8_t offset;
};

class IPCamera : public Device
{
public:
    IPCamera();
    explicit IPCamera(Bootstrap *boots);
    ~IPCamera() override;

    String name() override;
    void init() override;
    void publish(JsonObject &writer, uint8_t attempt_count, const String &payloadId) override;

    void loop() override;
    void read() override;

    uint8_t maintenanceCount() override;
    uint8_t paramCount() override;
    void clear() override;
    void print() override;
    size_t buffSize() override;
    void restoreDefaults() override;

private:
    // Boot/persist
    Bootstrap *_boots = nullptr;
    IPCameraConfigStruct _config{1, 0};
    uint16_t _eepromAddress = 0;

    // Cadence
    int _offset = 1;
    uint8_t _offsetCount = 0;

    // Flags
    volatile bool _snapRequested = false;

    // Tunnel
    SocketConfig _sockCfg;
    WssRtspTunnel _tunnel;

    // Worker task + queue
    struct SnapJob
    {
        uint32_t timeoutMs;
        bool forced;        // snapNow()
        char payloadId[32]; // null-terminated
    };

    QueueHandle_t _q = nullptr;
    TaskHandle_t _task = nullptr;
    bool _taskRunning = false;

    // Backoff (prevents thrash when server is down)
    uint32_t _backoffMs = 1000;
    const uint32_t _backoffMaxMs = 30000;
    uint32_t _nextAllowedStart = 0;

    // Methods
    void cloudFunctions();
    void requestAddress();
    bool validAddress();
    void pullStoredConfig();
    void saveOffset(uint8_t offset);
    void setOffsetValue();
    bool readySend();

    void enqueueSnap(uint32_t timeoutMs, bool forced, const String &payloadId);

    static void taskThunk(void *arg);
    void taskMain();

    // cloud callbacks
    int setOffsetMultiple(String value);
    int snapNow(String value);
};