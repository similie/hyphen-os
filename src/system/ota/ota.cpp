#include "system/ota.h"

OTAUpdate::OTAUpdate()
{
}

void OTAUpdate::setup()
{
    Hyphen.function("otaUpdate", &OTAUpdate::onUpdateMessage, this);
}

void OTAUpdate::loop()
{

    if (!persistenceInitialized && Hyphen.ready())
    {
        persistenceInitialized = true;
        char buildBuffer[BUILD_ID_MAX_LEN] = {0};
        if (Persist.get(PERSISTENCE_KEY, buildBuffer) && buildBuffer[0] != '\0')
        {
            Utils::log(UTILS_LOG_TAG, StringFormat("🔄 Last applied build ID: %s\n", buildBuffer));
            Hyphen.publish(ackTopic, "{\"status\":\"complete\",\"build\":\"" + String(buildBuffer) + "\"}");
            char empty[BUILD_ID_MAX_LEN] = {0};
            Persist.put(PERSISTENCE_KEY, empty);
        }
    }
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
        // sslClient.setClient(&Hyphen.hyConnect().secureClient());
        Hyphen.hyConnect().getSecureClient().setCACert(getCaCertificate().c_str());
        // Optional: sslClient.setInsecure(); // if no root CA yet
        return Hyphen.hyConnect().getSecureClient();
    }
    return Hyphen.hyConnect().getClient();
}

void OTAUpdate::parseDetailsAndSendUpdate()
{
    updateReady = false;

    // 1) Outer JSON: contains signature + payload/encrypted
    JsonDocument outerDoc;
    DeserializationError err1 = deserializeJson(outerDoc, receivedPayload);
    if (err1)
    {
        Serial.println("⚠️ OTA outer JSON parse failed");
        return;
    }

    // Extract signature
    String signature = outerDoc["signature"].as<String>();

    // Extract payloadJson (either encrypted or plain)
    String payloadJson = "";
    if (outerDoc["encrypted"].is<const char *>())
    {
        String encrypted = outerDoc["encrypted"].as<String>();
        if (!crypto.decryptPayload(encrypted, payloadJson))
        {
            Serial.println("OTA decrypt failed");
            return;
        }
    }
    else if (outerDoc["payload"].is<const char *>())
    {
        payloadJson = outerDoc["payload"].as<String>();
    }
    else
    {
        Serial.println("OTA missing payload/encrypted");
        return;
    }

    // 2) Verify signature
    if (!crypto.verifySignature(payloadJson, signature))
    {
        Serial.println("OTA payload signature invalid");
        return;
    }
    Serial.println("OTA payload signature valid");

    // 3) Parse inner payload
    JsonDocument doc;
    DeserializationError err2 = deserializeJson(doc, payloadJson);
    if (err2)
    {
        Serial.println("OTA inner JSON parse failed");
        return;
    }

    const char *devId = doc["device"];
    const char *buildId = doc["build"];
    const char *host = doc["host"];
    const char *url = doc["url"];
    const char *token = doc["token"];
    uint16_t port = uint16_t(doc["port"] | 80U);
    const char *timestamp = doc["timestamp"];
    const char *nonce = doc["nonce"];

    // 4) Validate fields
    if (!host || !url)
    {
        Serial.println("OTA missing host or URL");
        return;
    }

    if (String(devId) != Hyphen.deviceID())
    {
        Serial.println("OTA deviceId mismatch");
        return;
    }

    // Optionally: check timestamp freshness and nonce uniqueness here
    Utils::log(UTILS_LOG_TAG, StringFormat("OTA from %s:%u %s (build %s)\n", host, port, url, buildId));
    receivedPayload = ""; // clear sensitive data
    // 5) Proceed to update
    downloadAndUpdate(host, url, token, port, buildId);
}

void OTAUpdate::downloadAndUpdate(const char *host, const char *firmwareUrl, const char *token, uint16_t port, const char *buildid)
{
    Client &client = getClient(port);
    HttpClient http(client, host, port);

    Serial.println("Connecting to firmware host...");

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
        Utils::log(UTILS_LOG_TAG, "OTA HTTP %d\n", statusCode);
        http.stop();
        Hyphen.publish(ackTopic, "{\"status\":\"failed\",\"code\":404}");
        return;
    }

    int contentLength = http.contentLength();
    if (contentLength <= 0)
    {
        Utils::log(UTILS_LOG_TAG, "Content length missing");
        http.stop();
        Hyphen.publish(ackTopic, "{\"status\":\"failed\",\"code\":411}");
        return;
    }

    if (!Update.begin(contentLength))
    {
        Utils::log(UTILS_LOG_TAG, "Not enough space for OTA");
        http.stop();
        Hyphen.publish(ackTopic, "{\"status\":\"failed\",\"code\":507}");
        return;
    }

    Utils::log(UTILS_LOG_TAG, "⬇️ Starting OTA (%d bytes)\n", contentLength);
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
                    Utils::log(UTILS_LOG_TAG, StringFormat("… %d/%d bytes\n", written, contentLength));
                    lastProgress = millis();
                    Hyphen.publish(ackTopic, "{\"status\":\"progress\", \"progress\":" + String(written * 100 / contentLength) + "}");
                }
            }
            yield();
            lastReadMillis = millis();
        }
        else
        {
            if (millis() - lastReadMillis > 5000)
            {
                Serial.println("No data for 5 seconds, breaking");
                break;
            }
            coreDelay(1);
            // yield();
        }
    }

    // Attempt clean shutdown of connection
    http.stop();
    if (client.connected())
    {
        client.stop();
    }

    Serial.println("⬇️ OTA download complete...");

    if (Update.end() && Update.isFinished())
    {
        Serial.println("✅ OTA successful, rebooting...");
        Hyphen.publish(ackTopic, "{\"status\":\"rebooting\"}");
        char buildBuffer[BUILD_ID_MAX_LEN] = {0};
        strncpy(buildBuffer, buildid, BUILD_ID_MAX_LEN - 1);
        Persist.put(PERSISTENCE_KEY, buildBuffer);
        coreDelay(1000);
        ESP.restart();
    }
    else
    {
        Utils::log(UTILS_LOG_TAG, "❌ OTA failed: %d\n", Update.getError());
        Hyphen.publish(ackTopic, "{\"status\":\"failed\",\"code\":500}");
    }
}