#include "system/ota.h"

OTAUpdate::OTAUpdate() {}

void OTAUpdate::setup()
{
    Hyphen.function("otaUpdate", &OTAUpdate::onUpdateMessage, this);
}

void OTAUpdate::loop()
{
    if (!updateReady)
        return;

    // avoid hammering in case of repeat failures
    if (millis() - lastAttempt < retryInterval)
        return;
    lastAttempt = millis();

    parseDetailsAndSendUpdate();
}

int OTAUpdate::onUpdateMessage(String payload)
{
    receivedPayload = payload;
    tick.once(3, &OTAUpdate::runUpdateCallback, this); // short delay
    return 1;
}

void OTAUpdate::runUpdateCallback(OTAUpdate *instance)
{
    instance->updateReady = true;
}

String OTAUpdate::getCaCertificate()
{
    size_t ca_len = _binary_src_certs_isrgrootx1_pem_end - _binary_src_certs_isrgrootx1_pem_start;
    String caContent((const char *)_binary_src_certs_isrgrootx1_pem_start, ca_len);
    return caContent;
}

Client &OTAUpdate::getClient(uint16_t port)
{
    if (port == 443)
    {
        // Client secureClient = Hyphen.hyConnect().getClient();
        sslClient.setClient(&Hyphen.hyConnect().getClient());
        sslClient.setCACert(getCaCertificate().c_str());
        // Optional: sslClient.setInsecure(); // if no root CA yet
        return sslClient;
    }
    return Hyphen.hyConnect().getClient();
}

void OTAUpdate::parseDetailsAndSendUpdate()
{
    updateReady = false;
    JsonDocument doc;
    if (deserializeJson(doc, receivedPayload))
    {
        Serial.println("⚠️ OTA payload JSON parse failed");
        return;
    }

    const char *host = doc["host"];
    const char *url = doc["url"];
    const char *token = doc["token"];
    uint16_t port = doc["port"] | 80;

    if (!host || !url)
    {
        Serial.println("⚠️ OTA missing host or URL");
        return;
    }

    Serial.printf("📦 OTA from %s:%u%s\n", host, port, url);
    downloadAndUpdate(host, url, token, port);
}

void OTAUpdate::downloadAndUpdate(const char *host, const char *firmwareUrl, const char *token, uint16_t port)
{
    Client &client = getClient(port);
    HttpClient http(client, host, port);

    Serial.println("🔗 Connecting to firmware host...");

    http.beginRequest();
    http.get(firmwareUrl);

    if (token && strlen(token) > 0)
    {
        String authHeader = "Bearer ";
        authHeader += token;
        http.sendHeader("Authentication", authHeader);
    }

    http.sendHeader("Accept", "application/octet-stream");
    http.endRequest();

    Hyphen.publish(ackTopic, "{\"status\":\"started\"}");

    int statusCode = http.responseStatusCode();
    if (statusCode != 200)
    {
        Serial.printf("❌ OTA HTTP %d\n", statusCode);
        http.stop();
        Hyphen.publish(ackTopic, "{\"status\":\"failed\",\"code\":404}");
        return;
    }

    int contentLength = http.contentLength();
    if (contentLength <= 0)
    {
        Serial.println("❌ Content length missing");
        http.stop();
        Hyphen.publish(ackTopic, "{\"status\":\"failed\",\"code\":411}");
        return;
    }

    if (!Update.begin(contentLength))
    {
        Serial.println("❌ Not enough space for OTA");
        http.stop();
        Hyphen.publish(ackTopic, "{\"status\":\"failed\",\"code\":507}");
        return;
    }

    Serial.printf("⬇️ Starting OTA (%d bytes)\n", contentLength);
    const size_t BUFF_SIZE = 512 * 8;

    uint8_t buff[BUFF_SIZE];
    int written = 0;

    // ✅ Use the original client directly — http uses it internally
    unsigned long lastProgress = millis();

    // uint8_t buff[1024];
    // int written = 0;
    unsigned long lastReadMillis = millis();

    // ✅ THIS IS THE CORRECT WAY TO GET THE BODY STREAM:
    // ** Use the underlying client stream ** (not http.responseBody())
    while (client.connected() && written < contentLength)
    {
        int avail = client.available();
        if (avail > 0)
        {
            int toRead = (avail > BUFF_SIZE ? BUFF_SIZE : avail);
            int len = client.read(buff, toRead);
            if (len > 0)
            {
                Update.write(buff, len);
                written += len;

                if (millis() - lastProgress > 1000)
                {
                    Serial.printf("… %d/%d bytes\n", written, contentLength);
                    lastProgress = millis();
                    // Hyphen.publish(ackTopic, "{\"status\":\"started\"}");
                }
            }
            yield();
            lastReadMillis = millis();
        }
        else
        {
            if (millis() - lastReadMillis > 5000)
            {
                Serial.println("⚠️ No data for 5 seconds, breaking");
                break;
            }
            coreDelay(1);
            // yield();
        }
    }
    // Stream &stream = http.stre

    // while (written < contentLength)
    // {
    //     int len = stream.readBytes(buff, sizeof(buff));
    //     if (len <= 0)
    //         break;

    //     // Update.write(buff, len);
    //     written += len;

    //     if (millis() - lastProgress > 1000)
    //     {
    //         Serial.printf("... %d/%d bytes\n", written, contentLength);
    //         lastProgress = millis();
    //     }
    // }
    // Stream &stream = http.responseBody();

    // while (client.connected() && written < contentLength)
    // {
    //     int avail = client.available();
    //     if (avail > 0)
    //     {
    //         int len = client.read(buff, min((int)sizeof(buff), avail));
    //         if (len > 0)
    //         {
    //             // Update.write(buff, len);
    //             written += len;
    //         }

    //         if (millis() - lastProgress > 1000)
    //         {
    //             Serial.printf("  ... %d/%d bytes\r\n", written, contentLength);
    //             lastProgress = millis();
    //         }
    //     }
    //     delay(1);
    // }

    Serial.println("⬇️ OTA download complete...");

    if (Update.end() && Update.isFinished())
    {
        Serial.println("✅ OTA successful, rebooting...");
        Hyphen.publish(ackTopic, "{\"status\":\"ok\"}");
        delay(1000);
        ESP.restart();
    }
    else
    {
        Serial.printf("❌ OTA failed: %d\n", Update.getError());
        Hyphen.publish(ackTopic, "{\"status\":\"failed\",\"code\":500}");
    }

    http.stop();
}