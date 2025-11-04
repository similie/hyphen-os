#ifndef OTA_UPDATE_H
#define OTA_UPDATE_H

#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoHttpClient.h>

#include <Update.h>
#include "Hyphen.h"
/**
 * @brief NEEDS TESTING. This class is used to update the firmware of the device
 *
 *
 *
 */

extern const uint8_t _binary_src_certs_isrgrootx1_pem_start[] asm("_binary_src_certs_isrgrootx1_pem_start");
extern const uint8_t _binary_src_certs_isrgrootx1_pem_end[] asm("_binary_src_certs_isrgrootx1_pem_end");
class OTAUpdate
{
public:
    OTAUpdate();
    void setup();
    void loop();
    Ticker tick;

private:
    String receivedPayload = "";
    String getCaCertificate();
    bool updateReady = false;
    void parseDetailsAndSendUpdate();
    static void runUpdateCallback(OTAUpdate *);
    SSLClientESP32 sslClient;
    const String mqttTopic = String(MQTT_TOPIC_BASE) + "Config/OTA/" + Hyphen.deviceID();
    const String ackTopic = String(MQTT_TOPIC_BASE) + "Config/OTA/ack/" + Hyphen.deviceID();
    int onUpdateMessage(String);
    void downloadAndUpdate(const char *, const char *, const char *, uint16_t);
    Client &getClient(uint16_t);
    unsigned long lastAttempt = 0;
    const unsigned long retryInterval = 30000; // 30s safety delay
};

#endif