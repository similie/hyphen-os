#include "Hyphen.h"
#include "string.h"
#include <stdint.h>
#include <Adafruit_VC0706.h>
#include "device.h"
#include "relay.h"
#include "resources/bootstrap/bootstrap.h"
#include "resources/processors/LocalProcessor.h"

#define OFFSET_MULTIPLE 1

struct VideoCaptureStruct
{
    uint8_t version;
    uint8_t offset;
};

#ifndef video_capture_h
#define video_capture_h

class VideoCapture : public Device
{
private:
    VideoCaptureStruct config;
    uint16_t eepromAddress = 0;
    Relay relay = Relay(7, false);
    Adafruit_VC0706 cam = Adafruit_VC0706(&Serial1);
    TCPClient client;
    Bootstrap *boots;
    bool connected = false;
    bool cameraReady = false;
    String server = "images.similie.org";
    int port = 1399;
    int offset = OFFSET_MULTIPLE;
    u_int8_t offsetCount = 0;
    void cloudFunctions();
    void pullStoredConfig();
    void requestAddress();
    bool validAddress();
    void setOffsetValue();
    int setOffsetMultiple(String value);
    bool setupCamera();
    void preCaptureCamera();
    char *version;
    const u_int8_t VERSION_CHECK = 5;
    char *getVersion();
    bool connnectToServer();
    bool isConnected();
    void sendBuffer(uint8_t *buffer, uint8_t bytesToRead);
    void writeAString(String topic);
    String getEndHeader();
    String getHeader();
    void writeClose();
    void writeHead();
    uint16_t writeBuffer();
    void disconnect();
    bool snapPhoto();
    bool takeShot();
    bool shotStartUp();
    bool transmitImageData();
    bool readySend();
    void reset();
    bool on();
    bool off();

public:
    ~VideoCapture();
    VideoCapture();
    VideoCapture(Bootstrap *boots);
    String name();
    void read();
    void loop();
    uint8_t maintenanceCount();
    uint8_t paramCount();
    void clear();
    void print();
    size_t buffSize();
    void init();
    void restoreDefaults();
    void publish(JsonObject &writer, uint8_t attempt_count);
};

#endif