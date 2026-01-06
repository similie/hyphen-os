#include "system/device-security.h"

static String base64UrlEncode_(const uint8_t *data, size_t len)
{
    size_t olen = 0;
    const size_t maxOut = 4 * ((len + 2) / 3) + 1;
    unsigned char *tmp = (unsigned char *)malloc(maxOut);
    if (!tmp)
        return "";

    if (mbedtls_base64_encode(tmp, maxOut, &olen, data, len) != 0)
    {
        free(tmp);
        return "";
    }

    String out;
    out.reserve(olen);

    for (size_t i = 0; i < olen; i++)
    {
        char c = (char)tmp[i];
        if (c == '+')
            out += '-';
        else if (c == '/')
            out += '_';
        else if (c == '=')
            break; // strip padding
        else
            out += c;
    }

    free(tmp);
    return out;
}

static void sha256_(const uint8_t *data, size_t len, uint8_t out[32])
{
    mbedtls_sha256_context sha;
    mbedtls_sha256_init(&sha);
    mbedtls_sha256_starts_ret(&sha, 0);
    mbedtls_sha256_update_ret(&sha, data, len);
    mbedtls_sha256_finish_ret(&sha, out);
    mbedtls_sha256_free(&sha);
}

// --- MAIN FUNCTION ---
String DeviceSecurity::makePayloadId(const String &deviceId)
{
    (void)deviceId; // not needed anymore, but kept for API stability

    uint8_t rnd[12]; // 96 bits

    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context ctr;
    mbedtls_entropy_init(&entropy);
    mbedtls_ctr_drbg_init(&ctr);

    const char *pers = "hyphen_payload_id";
    if (mbedtls_ctr_drbg_seed(
            &ctr,
            mbedtls_entropy_func,
            &entropy,
            (const unsigned char *)pers,
            strlen(pers)) != 0)
    {
        mbedtls_ctr_drbg_free(&ctr);
        mbedtls_entropy_free(&entropy);
        return "";
    }

    if (mbedtls_ctr_drbg_random(&ctr, rnd, sizeof(rnd)) != 0)
    {
        mbedtls_ctr_drbg_free(&ctr);
        mbedtls_entropy_free(&entropy);
        return "";
    }

    mbedtls_ctr_drbg_free(&ctr);
    mbedtls_entropy_free(&entropy);

    return String("p1_") + base64UrlEncode_(rnd, sizeof(rnd));
}

static void mbedErrToString(int ret, char *out, size_t outLen)
{
    if (!out || outLen == 0)
        return;
    mbedtls_strerror(ret, out, outLen);
}

static bool initPkFromEmbedded_(mbedtls_pk_context &pk)
{
    mbedtls_pk_init(&pk);

    const uint8_t *priv_start = _binary_src_certs_private_key_pem_start;
    const size_t priv_len = (size_t)(_binary_src_certs_private_key_pem_end - _binary_src_certs_private_key_pem_start);

    // Note: parse_key expects the PEM buffer to include null terminator in some builds.
    // Many embedded linkers include it; if not, mbedtls still usually parses fine.
    const int ret = mbedtls_pk_parse_key(&pk, priv_start, priv_len, nullptr, 0);
    if (ret != 0)
    {
        char buf[128];
        mbedErrToString(ret, buf, sizeof(buf));
        Serial.printf("❌ DeviceSecurity pk_parse_key failed: %s\n", buf);
        mbedtls_pk_free(&pk);
        return false;
    }
    return true;
}

static bool base64Encode_(const uint8_t *in, size_t inLen, String &outB64)
{
    size_t olen = 0;
    // mbedtls wants output buffer; compute upper bound
    // base64 len <= 4 * ceil(n/3) + 1
    const size_t maxOut = 4 * ((inLen + 2) / 3) + 1;

    // Use heap temporarily (signature is small, typically <= 256 bytes for RSA-2048)
    unsigned char *tmp = (unsigned char *)malloc(maxOut);
    if (!tmp)
        return false;

    const int ret = mbedtls_base64_encode(tmp, maxOut, &olen, in, inLen);
    if (ret != 0)
    {
        free(tmp);
        return false;
    }

    outB64 = String((const char *)tmp, olen);
    free(tmp);
    return true;
}

// ota.cpp
const char *DeviceSecurity::getCaCertificate()
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

bool DeviceSecurity::signSha256ToBase64(const String &message, String &outSigB64)
{
    outSigB64 = "";

    // SHA-256 hash of message
    uint8_t hash[32];
    mbedtls_sha256_context sha;
    mbedtls_sha256_init(&sha);
    mbedtls_sha256_starts_ret(&sha, 0);
    mbedtls_sha256_update_ret(&sha, (const unsigned char *)message.c_str(), message.length());
    mbedtls_sha256_finish_ret(&sha, hash);
    mbedtls_sha256_free(&sha);

    // Load private key
    mbedtls_pk_context pk;
    if (!initPkFromEmbedded_(pk))
        return false;

    // RNG (required for many sign operations)
    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context ctr;
    mbedtls_entropy_init(&entropy);
    mbedtls_ctr_drbg_init(&ctr);

    const char *pers = "hyphen_wss_sign";
    int ret = mbedtls_ctr_drbg_seed(&ctr, mbedtls_entropy_func, &entropy,
                                    (const unsigned char *)pers, strlen(pers));
    if (ret != 0)
    {
        char buf[128];
        mbedErrToString(ret, buf, sizeof(buf));
        Serial.printf("❌ DeviceSecurity ctr_drbg_seed failed: %s\n", buf);
        mbedtls_pk_free(&pk);
        mbedtls_ctr_drbg_free(&ctr);
        mbedtls_entropy_free(&entropy);
        return false;
    }

    // Sign hash with key
    uint8_t sig[512];
    size_t sigLen = 0;

    ret = mbedtls_pk_sign(&pk,
                          MBEDTLS_MD_SHA256,
                          hash,
                          0,
                          sig,
                          &sigLen,
                          mbedtls_ctr_drbg_random,
                          &ctr);

    if (ret != 0)
    {
        char buf[128];
        mbedErrToString(ret, buf, sizeof(buf));
        Serial.printf("❌ DeviceSecurity pk_sign failed: %s\n", buf);
        mbedtls_pk_free(&pk);
        mbedtls_ctr_drbg_free(&ctr);
        mbedtls_entropy_free(&entropy);
        return false;
    }

    // Base64 signature
    const bool ok = base64Encode_(sig, sigLen, outSigB64);

    mbedtls_pk_free(&pk);
    mbedtls_ctr_drbg_free(&ctr);
    mbedtls_entropy_free(&entropy);

    return ok;
}

bool DeviceSecurity::signToBase64(const String &message, String &outSigB64)
{
    // For clarity: this signs SHA-256(message) (recommended).
    return signSha256ToBase64(message, outSigB64);
}