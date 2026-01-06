#pragma once
#include <Arduino.h>
#include "connections/Connection.h" // SecureClient, Client
#include "system/socket-config.h"
#include "system/wss-socket-client.h"
#include "system/device-security.h"
#include "Hyphen.h"

class WssRtspTunnel
{
public:
    enum class State : uint8_t
    {
        IDLE,
        WSS_CONNECTING,
        TUNNELING,
        DONE,
        ERROR
    };

    explicit WssRtspTunnel(const SocketConfig &cfg);

    void begin();

    bool startSnapshot(SecureClient &tls, Client &cam, const String &payloadId);
    void loop(SecureClient &tls, Client &cam);
    void stop(SecureClient &tls, Client &cam);

    bool finished() const { return _state == State::DONE || _state == State::ERROR; }
    bool isOk() const;

    State state() const { return _state; }
    const char *lastError() const { return _lastError.c_str(); }

private:
    SocketConfig _cfg;
    WssSocketClient _wss;

    State _state = State::IDLE;
    uint32_t _startedAt = 0;
    String _lastError;
    String _payloadId;
    // --- RTSP first-request validation (server->camera) ---
    bool _rtspValidateEnabled = true; // only relevant while not validated
    bool _rtspValidated = false;
    size_t _rtspHdrLen = 0;
    static constexpr size_t RTSP_HDR_MAX = 1024;
    char _rtspHdrBuf[RTSP_HDR_MAX];

    bool _camConnected = false;
    bool _stopping = false;

    // Success heuristic
    uint32_t _bytesUp = 0;

    // Auth handshake state (client-side implemented now; server-side verification later)
    bool _ready = false;
    bool _helloSent = false;
    bool _authed = false;
    String _nonceB64; // from CHAL <nonceB64>

    void setError(const char *e) { _lastError = e ? e : ""; }

    bool camConnect(Client &cam);
    void camStop(Client &cam);
    void sendCamToServer(SecureClient &tls, Client &cam);
    bool rtspInspectServerToCam_(const uint8_t *data, size_t len);
    bool validateRtspHeader_(const char *hdr, size_t len) const;
    // Text protocol helpers
    String buildHello_();
    String buildAuth_(const String &nonceB64); // real signature, not stub
};