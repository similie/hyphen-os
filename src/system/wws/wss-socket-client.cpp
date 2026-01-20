#include "system/wss-socket-client.h"
#include <cstring>

void WssSocketClient::randomBytes(uint8_t *b, size_t n)
{
    for (size_t i = 0; i < n; i++)
        b[i] = (uint8_t)esp_random();
}

String WssSocketClient::base64Encode(const uint8_t *data, size_t len)
{
    static const char *b64 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    String out;
    out.reserve(((len + 2) / 3) * 4);

    for (size_t i = 0; i < len; i += 3)
    {
        const uint32_t a = (i + 0 < len) ? data[i + 0] : 0;
        const uint32_t b = (i + 1 < len) ? data[i + 1] : 0;
        const uint32_t c = (i + 2 < len) ? data[i + 2] : 0;

        const uint32_t triple = (a << 16) | (b << 8) | c;

        out += b64[(triple >> 18) & 0x3F];
        out += b64[(triple >> 12) & 0x3F];
        out += (i + 1 < len) ? b64[(triple >> 6) & 0x3F] : '=';
        out += (i + 2 < len) ? b64[(triple >> 0) & 0x3F] : '=';
    }
    return out;
}

bool WssSocketClient::connect(SecureClient &tls,
                              const char *host,
                              uint16_t port,
                              const char *path,
                              bool insecure)
{
    _up = false;
    _err = "";

    if (insecure)
    {
        tls.setInsecure();
    }
    else
    {
        const char *caCert = DeviceSecurity::getCaCertificate();
        if (caCert == nullptr || strlen(caCert) == 0)
        {
            setErr("no_ca_certificate");
            return false;
        }
        // Serial.printf("Using CA Certificate %s: ", host);
        // Serial.println(caCert);
        tls.setCACert(caCert);
    }

    if (!tls.connect(host, port))
    {
        setErr("tcp_tls_connect_failed");
        return false;
    }

    // Sec-WebSocket-Key
    uint8_t keyRaw[16];
    randomBytes(keyRaw, sizeof(keyRaw));
    const String keyB64 = base64Encode(keyRaw, sizeof(keyRaw));

    String req;
    req += "GET ";
    req += path;
    req += " HTTP/1.1\r\n";
    req += "Host: ";
    req += host;
    req += "\r\n";
    req += "Upgrade: websocket\r\n";
    req += "Connection: Upgrade\r\n";
    req += "Sec-WebSocket-Version: 13\r\n";
    req += "Sec-WebSocket-Key: ";
    req += keyB64;
    req += "\r\n\r\n";

    tls.write((const uint8_t *)req.c_str(), req.length());

    // Read response headers
    uint32_t start = millis();
    String hdr;
    while (millis() - start < 5000)
    {
        while (tls.available())
        {
            char c = (char)tls.read();
            hdr += c;
            if (hdr.endsWith("\r\n\r\n"))
                goto done;
        }
        delay(0);
    }

    setErr("ws_handshake_timeout");
    tls.stop();
    return false;

done:
    if (hdr.indexOf(" 101 ") < 0)
    {
        setErr("ws_handshake_not_101");
        tls.stop();
        return false;
    }

    _up = true;
    return true;
}

void WssSocketClient::disconnect(SecureClient &tls)
{
    if (tls.connected())
        tls.stop();
    _up = false;
}

bool WssSocketClient::sendFrame(SecureClient &tls, uint8_t opcode, const uint8_t *data, size_t len)
{
    if (!_up)
        return false;
    if (!tls.connected())
    {
        _up = false;
        return false;
    }

    // client->server MUST be masked
    uint8_t mask[4];
    randomBytes(mask, 4);

    // Header
    uint8_t h[14];
    size_t hl = 0;

    h[hl++] = (uint8_t)(0x80 | (opcode & 0x0F)); // FIN + opcode

    if (len <= 125)
    {
        h[hl++] = (uint8_t)(0x80 | (uint8_t)len);
    }
    else if (len <= 0xFFFF)
    {
        h[hl++] = (uint8_t)(0x80 | 126);
        h[hl++] = (uint8_t)((len >> 8) & 0xFF);
        h[hl++] = (uint8_t)(len & 0xFF);
    }
    else
    {
        setErr("ws_len_64_not_supported");
        return false;
    }

    // mask
    for (int i = 0; i < 4; i++)
        h[hl++] = mask[i];

    tls.write(h, hl);

    // masked payload in chunks
    uint8_t tmp[256];
    size_t offset = 0;

    while (offset < len)
    {
        const size_t chunk = (len - offset > sizeof(tmp)) ? sizeof(tmp) : (len - offset);
        for (size_t i = 0; i < chunk; i++)
            tmp[i] = data[offset + i] ^ mask[(offset + i) & 3];

        tls.write(tmp, chunk);
        offset += chunk;
        delay(0);
    }

    return true;
}

bool WssSocketClient::sendBin(SecureClient &tls, const uint8_t *data, size_t len)
{
    return sendFrame(tls, 0x2, data, len);
}

bool WssSocketClient::sendText(SecureClient &tls, const char *s)
{
    if (!s)
        return false;
    return sendFrame(tls, 0x1, (const uint8_t *)s, strlen(s));
}

void WssSocketClient::handlePing_(SecureClient &tls, const uint8_t *payload, size_t len)
{
    // respond with pong (opcode 0xA) with same payload
    (void)sendFrame(tls, 0xA, payload, len);
}

void WssSocketClient::poll(SecureClient &tls, BinHandler onBin, TextHandler onText)
{
    if (!_up)
        return;

    if (!tls.connected())
    {
        _up = false;
        return;
    }

    // need at least 2 bytes for header
    if (tls.available() < 2)
        return;

    // Read basic header
    const uint8_t b0 = (uint8_t)tls.read();
    const uint8_t b1 = (uint8_t)tls.read();

    const uint8_t opcode = (uint8_t)(b0 & 0x0F);
    const bool masked = (b1 & 0x80) != 0;
    uint64_t len = (uint64_t)(b1 & 0x7F);

    if (len == 126)
    {
        if (tls.available() < 2)
            return;
        const uint8_t hi = (uint8_t)tls.read();
        const uint8_t lo = (uint8_t)tls.read();
        len = ((uint16_t)hi << 8) | lo;
    }
    else if (len == 127)
    {
        setErr("ws_len_64_not_supported");
        _up = false;
        tls.stop();
        return;
    }

    uint8_t mask[4] = {0};
    if (masked)
    {
        if (tls.available() < 4)
            return;
        for (int i = 0; i < 4; i++)
            mask[i] = (uint8_t)tls.read();
    }

    // Keep it bounded
    static uint8_t buf[2048];
    if (len > sizeof(buf))
    {
        setErr("ws_frame_too_big");
        for (uint64_t i = 0; i < len; i++)
        {
            while (!tls.available())
                delay(0);
            (void)tls.read();
        }
        return;
    }

    for (uint64_t i = 0; i < len; i++)
    {
        while (!tls.available())
            delay(0);
        uint8_t v = (uint8_t)tls.read();
        if (masked)
            v ^= mask[i & 3];
        buf[i] = v;
    }

    // Dispatch
    if (opcode == 0x2)
    {
        if (onBin)
            onBin(buf, (size_t)len);
        return;
    }

    if (opcode == 0x1)
    {
        if (onText)
        {
            // Convert to String safely
            String s;
            s.reserve((size_t)len + 1);
            for (size_t i = 0; i < (size_t)len; i++)
                s += (char)buf[i];
            onText(s);
        }
        return;
    }

    if (opcode == 0x9)
    {
        // ping
        handlePing_(tls, buf, (size_t)len);
        return;
    }

    if (opcode == 0x8)
    {
        // close
        _up = false;
        tls.stop();
        return;
    }

    // Ignore pong / continuation / etc for now
}