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
class OTAUpdate
{
public:
    OTAUpdate();
    void setup();
    void loop();

private:
    SSLClientESP32 sslClient;
    const String mqttTopic = String(MQTT_TOPIC_BASE) + "Config/OTA/" + Hyphen.deviceID();
    void onMQTTMessage(const char *, const char *);
    void downloadAndUpdate(const char *, const char *, uint16_t);
    Client &getClient(uint16_t);
};

#endif