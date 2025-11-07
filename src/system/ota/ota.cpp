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

    // parseDetailsAndSendUpdate();
    startOtaTask();
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

String OTAUpdate::getCaCertificate(bool v)
{
    size_t ca_len = _binary_src_certs_isrgrootx1_pem_end - _binary_src_certs_isrgrootx1_pem_start;
    String caContent((const char *)_binary_src_certs_isrgrootx1_pem_start, ca_len);
    return caContent;
}

// ota.cpp
const char *OTAUpdate::getCaCertificate()
{
    static char *s_ca = nullptr;
    if (!s_ca)
    {
        size_t ca_len = _binary_src_certs_isrgrootx1_pem_end - _binary_src_certs_isrgrootx1_pem_start;
        s_ca = (char *)malloc(ca_len + 1);
        memcpy(s_ca, _binary_src_certs_isrgrootx1_pem_start, ca_len);
        s_ca[ca_len] = '\0'; // ensure PEM is NUL-terminated
    }
    return s_ca;
}

Client &OTAUpdate::getClient(uint16_t port)
{
    if (port == 443)
    {
        SecureClient &c = Hyphen.hyConnect().newSecureClient();
        c.setCACert(getCaCertificate());
        return c;
    }
    return Hyphen.hyConnect().newClient();
}

void OTAUpdate::parseDetailsAndSendUpdate()
{
    updateReady = false;

    // 1) Outer JSON: contains signature + payload/encrypted
    JsonDocument outerDoc;
    DeserializationError err1 = deserializeJson(outerDoc, receivedPayload);
    if (err1)
    {
        Utils::log(UTILS_LOG_TAG, "OTA outer JSON parse failed");
        Hyphen.publish(ackTopic, "{\"status\":\"failed\",\"code\":500,\"error\":\"Outer JSON parse failed\"}");
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
            Utils::log(UTILS_LOG_TAG, "OTA decrypt failed");
            Hyphen.publish(ackTopic, "{\"status\":\"failed\",\"code\":500,\"error\":\"Decryption failed\"}");
            return;
        }
    }
    else if (outerDoc["payload"].is<const char *>())
    {
        payloadJson = outerDoc["payload"].as<String>();
    }
    else
    {
        Utils::log(UTILS_LOG_TAG, "OTA missing payload/encrypted");
        Hyphen.publish(ackTopic, "{\"status\":\"failed\",\"code\":500,\"error\":\"Missing payload/encrypted\"}");
        return;
    }

    // 2) Verify signature
    if (!crypto.verifySignature(payloadJson, signature))
    {
        Utils::log(UTILS_LOG_TAG, "OTA payload signature invalid");
        Hyphen.publish(ackTopic, "{\"status\":\"failed\",\"code\":500,\"error\":\"Invalid signature\"}");
        return;
    }
    Utils::log(UTILS_LOG_TAG, "OTA payload signature valid");

    // 3) Parse inner payload
    JsonDocument doc;
    DeserializationError err2 = deserializeJson(doc, payloadJson);
    if (err2)
    {
        Utils::log(UTILS_LOG_TAG, "OTA inner JSON parse failed");
        Hyphen.publish(ackTopic, "{\"status\":\"failed\",\"code\":500,\"error\":\"Inner JSON parse failed\"}");
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
        Utils::log(UTILS_LOG_TAG, "OTA missing host or URL");
        Hyphen.publish(ackTopic, "{\"status\":\"failed\",\"code\":500,\"error\":\"Missing host or url\"}");
        return;
    }

    if (!isAllowedHost(host))
    {
        Utils::log(UTILS_LOG_TAG, "OTA host not allowed");
        Hyphen.publish(ackTopic, "{\"status\":\"failed\",\"code\":500,\"error\":\"Host not allowed\"}");
        return;
    }

    if (String(devId) != Hyphen.deviceID())
    {
        Utils::log(UTILS_LOG_TAG, "OTA deviceId mismatch");
        Hyphen.publish(ackTopic, "{\"status\":\"failed\",\"code\":500,\"error\":\"DeviceId mismatch\"}");
        return;
    }

    // Optionally: check timestamp freshness and nonce uniqueness here
    Utils::log(UTILS_LOG_TAG, StringFormat("OTA from %s:%u %s (build %s)\n", host, port, url, buildId));
    receivedPayload = ""; // clear sensitive data
    // 5) Proceed to update
    downloadAndUpdate(host, url, token, port, buildId);
}

void OTAUpdate::startOtaTask()
{
    updateReady = false; // we are consuming this trigger now

    xTaskCreatePinnedToCore(
        [](void *param)
        {
            OTAUpdate *self = static_cast<OTAUpdate *>(param);
            self->parseDetailsAndSendUpdate();
            vTaskDelete(NULL); // kill task when done
        },
        "OTA_UPDATE_TASK",
        24576, // <-- 16 KB stack for TLS handshake (safe)
        this,
        4, // priority (low)
        NULL,
        1 // APP CPU core
    );
}

bool OTAUpdate::isAllowedHost(const char *host)
{
    if (!host || !*host)
        return false;

    // Always allow localhost explicitly
    if (strcmp(host, "localhost") == 0 ||
        strcmp(host, "127.0.0.1") == 0 ||
        strcmp(host, "0.0.0.0") == 0)
        return true;

#ifdef OTA_ALLOWED_HOSTS
    // Example: "hyphen-api.similie.com,api.mycompany.com"
    const char *allowed = OTA_ALLOWED_HOSTS;
    const char *start = allowed;

    while (*start)
    {
        // Find the next comma or end
        const char *end = strchr(start, ',');
        size_t len = end ? (size_t)(end - start) : strlen(start);

        // Compare the host with this entry (case-sensitive or insensitive)
        if (strncmp(host, start, len) == 0 && host[len] == '\0')
        {
            return true; // MATCH ✅
        }

        // Move to the next entry
        if (!end)
            break;
        start = end + 1;
        while (*start == ' ' || *start == '\t')
            start++; // skip whitespace
    }

    // Not found → reject ❌
    return false;
#else
    // If not defined → allow all ✅
    return true;
#endif
}

void OTAUpdate::downloadAndUpdate(const char *host, const char *firmwareUrl, const char *token, uint16_t port, const char *buildid)
{
    Hyphen.publish(ackTopic, "{\"status\":\"started\"}");

    otaRunning = true;
    if (!maintainConnection())
    {
        Hyphen.hyConnect().pause();
    }

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

    int statusCode = http.responseStatusCode();
    if (statusCode != 200)
    {
        Utils::log(UTILS_LOG_TAG, "OTA HTTP %d\n", statusCode);
        http.stop();
        Hyphen.publish(ackTopic, "{\"status\":\"failed\",\"code\":404,\"error\":\"Host Not found\"}");
        otaRunning = false;
        return;
    }

    int contentLength = http.contentLength();
    if (contentLength <= 0)
    {
        Utils::log(UTILS_LOG_TAG, "Content length missing");
        http.stop();
        Hyphen.publish(ackTopic, "{\"status\":\"failed\",\"code\":411,\"error\":\"Content Length Required\"}");
        otaRunning = false;
        return;
    }

    if (!Update.begin(contentLength))
    {
        Utils::log(UTILS_LOG_TAG, "Not enough space for OTA");
        http.stop();
        Hyphen.publish(ackTopic, "{\"status\":\"failed\",\"code\":507,\"error\":\"Insufficient Storage\"}");
        otaRunning = false;
        return;
    }

    Utils::log(UTILS_LOG_TAG, StringFormat("⬇️ Starting OTA (%d bytes)\n", contentLength));
    const size_t BUFF_SIZE = 2920;

    uint8_t buff[BUFF_SIZE];
    int written = 0;

    // unsigned long lastProgress = millis();

    unsigned long lastReadMillis = millis();
    unsigned long lastProgressBytes = 0;
    while (client.connected() && written < contentLength)
    {
        int avail = client.available();
        if (avail > 0)
        {
            // int toRead = (avail > BUFF_SIZE ? BUFF_SIZE : avail);
            // int len = client.read(buff, toRead);
            int len = client.readBytes(buff, BUFF_SIZE);
            if (len > 0)
            {
                Update.write(buff, len);
                written += len;

                if (written - lastProgressBytes >= (16 * 1024))
                {
                    lastProgressBytes = written;
                    Utils::log(UTILS_LOG_TAG, StringFormat("… %d/%d bytes\n", written, contentLength));
                    // lastProgress = millis();
                    // Hyphen.publish(ackTopic, "{\"status\":\"progress\", \"progress\":" + String(written * 100 / contentLength) + "}");
                }
            }
            // yield();
            lastReadMillis = millis();
        }
        else
        {
            if (millis() - lastReadMillis > 30000)
            {
                Utils::log(UTILS_LOG_TAG, "No data for 30 seconds, breaking");
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

    Utils::log(UTILS_LOG_TAG, "⬇️ OTA download complete...");

    if (Update.end() && Update.isFinished())
    {
        Utils::log(UTILS_LOG_TAG, "✅ OTA successful, rebooting...");
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
        if (!maintainConnection())
        {
            Hyphen.hyConnect().resume();
        }

        Hyphen.publish(ackTopic, "{\"status\":\"failed\",\"code\":500,\"error\":\"Update Failed\"}");
        otaRunning = false;
    }
}

bool OTAUpdate::maintainConnection()
{
    return otaRunning && Hyphen.hyConnect().getConnectionClass() == ConnectionClass::WIFI;
}

bool OTAUpdate::updating()
{
    return otaRunning;
}