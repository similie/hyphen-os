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
    const String mqttTopic = String(MQTT_TOPIC_BASE) + "Config/OTA/" + Hyphen.deviceID();
    void onMQTTMessage(const char *topic, const char *payload);
    void downloadAndUpdate(const char *firmwareUrl, uint16_t port);
};

#endif