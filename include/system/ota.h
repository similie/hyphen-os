#ifndef OTA_UPDATE_H
#define OTA_UPDATE_H

#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoHttpClient.h>
#include <mbedtls/pk.h>
#include <mbedtls/sha256.h>
#include <mbedtls/base64.h>
#include <mbedtls/x509_crt.h>
#include "resources/utils/utils.h"
#include <Update.h>
#include "Hyphen.h"
/**
 * @brief NEEDS TESTING. This class is used to update the firmware of the device
 *
 *
 *
 */
#define BUILD_ID_MAX_LEN 64
extern const uint8_t _binary_src_certs_isrgrootx1_pem_start[] asm("_binary_src_certs_isrgrootx1_pem_start");
extern const uint8_t _binary_src_certs_isrgrootx1_pem_end[] asm("_binary_src_certs_isrgrootx1_pem_end");
// Externs for your embedded certs
extern const uint8_t _binary_src_certs_device_cert_pem_start[] asm("_binary_src_certs_device_cert_pem_start");
extern const uint8_t _binary_src_certs_device_cert_pem_end[] asm("_binary_src_certs_device_cert_pem_end");

extern const uint8_t _binary_src_certs_private_key_pem_start[] asm("_binary_src_certs_private_key_pem_start");
extern const uint8_t _binary_src_certs_private_key_pem_end[] asm("_binary_src_certs_private_key_pem_end");

class OTACrypto
{
public:
    // Verify signature: payloadJson is the JSON string you received, signatureBase64 is the base64 signature
    bool verifySignature(const String &payloadJson, const String &signatureBase64)
    {
        int ret;
        mbedtls_x509_crt cert;
        mbedtls_x509_crt_init(&cert);

        const uint8_t *cert_start = _binary_src_certs_device_cert_pem_start;
        size_t cert_len = _binary_src_certs_device_cert_pem_end - _binary_src_certs_device_cert_pem_start;

        ret = mbedtls_x509_crt_parse(&cert, cert_start, cert_len);
        if (ret != 0)
        {
            Serial.printf("❌ x509_crt_parse failed: -0x%04x\n", -ret);
            mbedtls_x509_crt_free(&cert);
            return false;
        }

        // Hash the payload
        unsigned char hash[32];
        ret = mbedtls_sha256_ret((const unsigned char *)payloadJson.c_str(),
                                 payloadJson.length(), hash, 0);
        if (ret != 0)
        {
            Serial.printf("❌ sha256_ret failed: -0x%04x\n", -ret);
            mbedtls_x509_crt_free(&cert);
            return false;
        }

        // Decode signature
        unsigned char sig_buf[512];
        size_t sig_len = 0;
        ret = mbedtls_base64_decode(sig_buf, sizeof(sig_buf), &sig_len,
                                    (const unsigned char *)signatureBase64.c_str(),
                                    signatureBase64.length());
        if (ret != 0)
        {
            Serial.printf("❌ base64_decode failed: -0x%04x\n", -ret);
            mbedtls_x509_crt_free(&cert);
            return false;
        }

        // Verify signature using public key inside the cert
        ret = mbedtls_pk_verify(&cert.pk, MBEDTLS_MD_SHA256,
                                hash, sizeof(hash),
                                sig_buf, sig_len);
        mbedtls_x509_crt_free(&cert);

        if (ret != 0)
        {
            Serial.printf("❌ signature verify failed: -0x%04x\n", -ret);
            return false;
        }

        return true;
    }

    // Decrypt payload: encryptedBase64 is the base64-encoded encrypted payload
    // This assumes you encrypted with the device's public key and here you decrypt with its private key.
    // If you are *not* encrypting the payload, you can skip using this function.
    bool decryptPayload(const String &encryptedBase64, String &outPayload)
    {
        int ret;
        mbedtls_pk_context pk;
        mbedtls_pk_init(&pk);

        const uint8_t *priv_start = _binary_src_certs_private_key_pem_start;
        size_t priv_len = _binary_src_certs_private_key_pem_end - _binary_src_certs_private_key_pem_start;

        ret = mbedtls_pk_parse_key(&pk, priv_start, priv_len, NULL, 0);
        if (ret != 0)
        {
            char buf[128];
            mbedtls_strerror(ret, buf, sizeof(buf));
            Serial.printf("❌ pk_parse_key failed: %s\n", buf);
            mbedtls_pk_free(&pk);
            return false;
        }

        // Decode the base64-encoded encrypted data
        size_t enc_len = 0;
        size_t max_buf = 512; // adjust if needed
        unsigned char enc_buf[512];
        ret = mbedtls_base64_decode(enc_buf, max_buf, &enc_len,
                                    (const unsigned char *)encryptedBase64.c_str(), encryptedBase64.length());
        if (ret != 0)
        {
            char buf[128];
            mbedtls_strerror(ret, buf, sizeof(buf));
            Serial.printf("❌ base64_decode (encrypted) failed: %s\n", buf);
            mbedtls_pk_free(&pk);
            return false;
        }

        // Prepare output buffer for decrypted
        unsigned char dec_buf[512];
        size_t dec_len = 0;

        // RSA decryption: using pk context
        ret = mbedtls_pk_decrypt(&pk, enc_buf, enc_len,
                                 dec_buf, &dec_len, sizeof(dec_buf),
                                 NULL, NULL);
        if (ret != 0)
        {
            char buf[128];
            mbedtls_strerror(ret, buf, sizeof(buf));
            Serial.printf("❌ pk_decrypt failed: %s\n", buf);
            mbedtls_pk_free(&pk);
            return false;
        }

        // Convert decrypted bytes into String
        outPayload = String((char *)dec_buf, dec_len);

        mbedtls_pk_free(&pk);
        return true;
    }
};

class OTAUpdate
{
public:
    OTAUpdate();
    void setup();
    void loop();
    Ticker tick;

private:
    const char *PERSISTENCE_KEY = "last_build_id";
    String UTILS_LOG_TAG = "OTAUpdate";
    bool persistenceInitialized = false;
    OTACrypto crypto;
    String receivedPayload = "";
    String getCaCertificate();
    bool updateReady = false;
    void parseDetailsAndSendUpdate();
    static void runUpdateCallback(OTAUpdate *);
    // SecureClient sslClient
    const String mqttTopic = String(MQTT_TOPIC_BASE) + "Config/OTA/" + Hyphen.deviceID();
    const String ackTopic = String(MQTT_TOPIC_BASE) + "Config/OTA/ack/" + Hyphen.deviceID();
    int onUpdateMessage(String);
    void downloadAndUpdate(const char *, const char *, const char *, uint16_t, const char *buildid);
    Client &getClient(uint16_t);
    unsigned long lastAttempt = 0;
    const unsigned long retryInterval = 30000; // 30s safety delay
};

#endif