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
    uint16_t port = doc["port"];

    downloadAndUpdate(firmwareUrl, port);
}

Client &OTAUpdate::getClient(const char *firmwareUrl)
{
    String url = String(firmwareUrl);
    int protocolEnd = url.indexOf("://");
    String protocol = url.substring(0, protocolEnd);
    if (protocol == "http")
    {
        return Hyphen.hyConnect().getClient();
    }

    SSLClient sslClient;
    sslClient.setClient(&Hyphen.hyConnect().getClient());
    return sslClient;
}

void OTAUpdate::downloadAndUpdate(const char *firmwareUrl, uint16_t port)
{
    Client &client = Hyphen.hyConnect().getClient();
    HttpClient http = HttpClient(getClient(firmwareUrl), firmwareUrl, port);
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