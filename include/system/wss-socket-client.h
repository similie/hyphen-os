#pragma once
#include <Arduino.h>
#include <functional>
#include "connections/Connection.h" // SecureClient
#include "system/device-security.h"
class WssSocketClient
{
public:
    using BinHandler = std::function<void(const uint8_t *, size_t)>;
    using TextHandler = std::function<void(const String &)>;

    bool connect(SecureClient &tls,
                 const char *host,
                 uint16_t port,
                 const char *path,
                 bool insecure);

    void disconnect(SecureClient &tls);

    bool sendFrame(SecureClient &tls, uint8_t opcode, const uint8_t *data, size_t len);
    bool sendBin(SecureClient &tls, const uint8_t *data, size_t len);
    bool sendText(SecureClient &tls, const char *s);
    bool sendText(SecureClient &tls, const String &s) { return sendText(tls, s.c_str()); }

    // Pump inbound frames. Provide handlers as needed.
    // - onBin: called for opcode 0x2 frames
    // - onText: called for opcode 0x1 frames
    void poll(SecureClient &tls, BinHandler onBin, TextHandler onText = nullptr);

    bool isUp() const { return _up; }
    const char *lastError() const { return _err.c_str(); }

private:
    bool _up = false;
    String _err;

    void setErr(const char *e) { _err = e; }

    static void randomBytes(uint8_t *b, size_t n);
    static String base64Encode(const uint8_t *data, size_t len);

    // internal helpers
    void handlePing_(SecureClient &tls, const uint8_t *payload, size_t len);
};