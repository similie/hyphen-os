#pragma once
#include <Arduino.h>
#include <mbedtls/pk.h>
#include <mbedtls/md.h>
#include <mbedtls/base64.h>
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/entropy.h>
#include <mbedtls/sha256.h>
#include <mbedtls/error.h>

// Embedded private key symbols (same style as your OTA)
extern const uint8_t _binary_src_certs_private_key_pem_start[] asm("_binary_src_certs_private_key_pem_start");
extern const uint8_t _binary_src_certs_private_key_pem_end[] asm("_binary_src_certs_private_key_pem_end");

extern const uint8_t _binary_src_certs_isrgrootx1_pem_start[] asm("_binary_src_certs_isrgrootx1_pem_start");
extern const uint8_t _binary_src_certs_isrgrootx1_pem_end[] asm("_binary_src_certs_isrgrootx1_pem_end");
class DeviceSecurity
{
public:
    // Signs a message using the embedded device private key (PEM).
    static String makePayloadId(const String &deviceId);
    // Output is base64 signature.
    static bool signToBase64(const String &message, String &outSigB64);
    static const char *getCaCertificate();
    // Helper: SHA256(message) then sign.
    static bool signSha256ToBase64(const String &message, String &outSigB64);
};