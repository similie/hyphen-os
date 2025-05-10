#include "system/ota.h"

OTAUpdate::OTAUpdate()
{
}

void OTAUpdate::setup()
{

    Hyphen.hyConnect().subscribe(mqttTopic.c_str(), [this](const char *topic, const char *payload)
                                 { onMQTTMessage(topic, payload); });
}

void OTAUpdate::loop()
{
}

void OTAUpdate::onMQTTMessage(const char *topic, const char *payload)
{
    String message = String(payload);
    JsonDocument doc;
    deserializeJson(doc, message);
    // we will send json to the client for update
    const char *firmwareUrl = doc["url"];
    const char *host = doc["host"];
    if (!host || !firmwareUrl)
    {
        return;
    }

    uint16_t port = doc["port"];

    downloadAndUpdate(host, firmwareUrl, port);
}

Client &OTAUpdate::getClient(uint16_t port)
{
    if (port != 443)
    {
        return Hyphen.hyConnect().getClient();
    }
    sslClient.setClient(&Hyphen.hyConnect().getClient());
    return sslClient;
}

void OTAUpdate::downloadAndUpdate(const char *host, const char *firmwareUrl, uint16_t port)
{
    Client &client = getClient(port);
    HttpClient http = HttpClient(client, host, port);
    http.get(firmwareUrl);
    int statusCode = http.responseStatusCode();
    if (statusCode != 200)
    {
        Serial.printf("HTTP GET failed with status code: %d\n", statusCode);
        http.stop();
        return;
    }

    int contentLength = http.contentLength();
    if (contentLength <= 0)
    {
        Serial.println("Content-Length not defined");
        http.stop();
        return;
    }

    if (!Update.begin(contentLength))
    {
        Serial.println("Not enough space to begin OTA");
        http.stop();
        return;
    }

    int written = Update.writeStream(http);
    if (written != contentLength)
    {
        Serial.printf("Written only: %d/%d. Retry?\n", written, contentLength);
        Update.end();
        http.stop();
        return;
    }

    if (!Update.end())
    {
        Serial.printf("Update failed. Error #: %d\n", Update.getError());
        http.stop();
        return;
    }

    if (!Update.isFinished())
    {
        Serial.println("Update not finished.");
        http.stop();
        return;
    }

    Serial.println("Update successfully completed. Restarting...");
    ESP.restart();
}