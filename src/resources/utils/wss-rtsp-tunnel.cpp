#include "resources/utils/wss-rtsp-tunnel.h"

static bool startsWithAnyIgnoreCase(const String &s, const char *const *list, size_t n)
{
    for (size_t i = 0; i < n; i++)
    {
        const String p = String(list[i]);
        if (s.length() >= p.length() && s.substring(0, p.length()).equalsIgnoreCase(p))
            return true;
    }
    return false;
}

WssRtspTunnel::WssRtspTunnel(const SocketConfig &cfg) : _cfg(cfg) {}

// Validate a complete RTSP header block (up to \r\n\r\n)
bool WssRtspTunnel::validateRtspHeader_(const char *hdr, size_t len) const
{
    // Must contain RTSP/1.0 somewhere in request line
    // Extract first line
    size_t lineEnd = 0;
    while (lineEnd + 1 < len)
    {
        if (hdr[lineEnd] == '\r' && hdr[lineEnd + 1] == '\n')
            break;
        lineEnd++;
    }
    if (lineEnd == 0 || lineEnd + 1 >= len)
        return false;

    String firstLine = String(hdr).substring(0, (int)lineEnd);
    firstLine.trim();

    // Allowed methods (tight list)
    static const char *const kAllowed[] = {
        "OPTIONS ", "DESCRIBE ", "SETUP ", "PLAY ", "PAUSE ", "TEARDOWN ", "GET_PARAMETER "};

    if (!startsWithAnyIgnoreCase(firstLine, kAllowed, sizeof(kAllowed) / sizeof(kAllowed[0])))
        return false;

    if (firstLine.indexOf("RTSP/1.0") < 0)
        return false;

    // Require CSeq header somewhere in the header block
    // Cheap search (case-insensitive): check common forms
    String block = String(hdr);
    if (block.indexOf("\r\nCSeq:") < 0 && block.indexOf("\r\ncseq:") < 0)
        return false;

    return true;
}

// Feed server->camera bytes into the one-time inspector.
// Returns: true = OK to forward; false = reject (caller should close tunnel)
bool WssRtspTunnel::rtspInspectServerToCam_(const uint8_t *data, size_t len)
{
    if (!_rtspValidateEnabled || _rtspValidated)
        return true; // already accepted; don’t inspect further

    // Append until we see \r\n\r\n or hit max
    for (size_t i = 0; i < len; i++)
    {
        if (_rtspHdrLen + 1 >= RTSP_HDR_MAX)
        {
            // too big / never terminated -> reject
            return false;
        }

        _rtspHdrBuf[_rtspHdrLen++] = (char)data[i];
        _rtspHdrBuf[_rtspHdrLen] = '\0';

        // Detect end of header
        if (_rtspHdrLen >= 4)
        {
            const size_t n = _rtspHdrLen;
            if (_rtspHdrBuf[n - 4] == '\r' && _rtspHdrBuf[n - 3] == '\n' &&
                _rtspHdrBuf[n - 2] == '\r' && _rtspHdrBuf[n - 1] == '\n')
            {
                // We have a complete header block
                const bool ok = validateRtspHeader_(_rtspHdrBuf, _rtspHdrLen);
                if (!ok)
                    return false;

                _rtspValidated = true;
                _rtspValidateEnabled = false; // disable further inspection
                return true;
            }
        }
    }

    // Not enough data yet; allow forwarding (or you can “hold” until validated—see note below)
    return true;
}

void WssRtspTunnel::begin()
{
    // no-op (fits HyphenOS init pattern)
}

bool WssRtspTunnel::isOk() const
{
    if (_state == State::DONE)
        return true;

    // If we forwarded any camera bytes and then errored/disconnected,
    // treat as likely success (server probably already saved frame).
    if (_bytesUp > 0 && _state == State::ERROR)
        return true;

    return false;
}

String WssRtspTunnel::buildHello_()
{
    // HELLO <deviceId>
    const String deviceId = Hyphen.deviceID();
    // (Server will respond READY then CHAL <nonce> later)
    if (_payloadId.length() > 0)
    {
        return String("HELLO ") + _payloadId + " " + deviceId;
    }
    return String("HELLO ") + deviceId;
}

String WssRtspTunnel::buildAuth_(const String &nonceB64)
{
    // AUTH <deviceId> <sigB64>
    //
    // What we sign:
    //   "<deviceId>.<nonceB64>"
    //
    // Server verifies using the stored device public key.
    const String deviceId = Hyphen.deviceID();
    const String toSign = deviceId + "." + nonceB64;

    String sigB64;
    const bool ok = DeviceSecurity::signSha256ToBase64(toSign, sigB64);
    if (!ok)
        return String(""); // caller handles error

    return String("AUTH ") + deviceId + " " + sigB64;
}

bool WssRtspTunnel::startSnapshot(SecureClient &tls, Client &cam, const String &payloadId)
{
    if (!Hyphen.connected())
    {
        setError("no_network");
        _state = State::ERROR;
        return false;
    }

    (void)cam; // camera only connects after OPEN

    if (_state == State::WSS_CONNECTING || _state == State::TUNNELING)
        return true;

    _startedAt = millis();
    _lastError = "";
    _state = State::WSS_CONNECTING;

    _stopping = false;
    _bytesUp = 0;
    _payloadId = payloadId; // may be "" for manual snaps
    // reset auth state
    _ready = false;
    _helloSent = false;
    _authed = false;
    _nonceB64 = "";
    // reset RTSP validation state
    _rtspValidateEnabled = true;
    _rtspValidated = false;
    _rtspHdrLen = 0;

    const bool ok = _wss.connect(
        tls,
        _cfg.wssHost.c_str(),
        _cfg.wssPort,
        _cfg.wssPath.c_str(),
        _cfg.tlsInsecure);

    if (!ok)
    {
        setError(_wss.lastError());
        _state = State::ERROR;
        return false;
    }

    // Send HELLO immediately (server will use it to choose device folder/context)
    const String hello = buildHello_();
    if (!_wss.sendText(tls, hello))
    {
        setError("hello_send_failed");
        _wss.disconnect(tls);
        _state = State::ERROR;
        return false;
    }
    _helloSent = true;

    return true;
}

void WssRtspTunnel::stop(SecureClient &tls, Client &cam)
{
    if (_stopping)
        return;
    _stopping = true;

    camStop(cam);
    _wss.disconnect(tls);
    // do not overwrite _state here
}

void WssRtspTunnel::loop(SecureClient &tls, Client &cam)
{
    if (_state == State::DONE || _state == State::ERROR)
        return;

    if ((_state == State::WSS_CONNECTING || _state == State::TUNNELING) &&
        (millis() - _startedAt > _cfg.tunnelTimeoutMs))
    {
        setError("tunnel_timeout");
        camStop(cam);
        _wss.disconnect(tls);
        _state = State::ERROR;
        return;
    }

    if (!tls.connected())
    {
        setError("wss_disconnected");
        camStop(cam);
        _state = State::ERROR;
        return;
    }

    _wss.poll(
        tls,

        // -------- BIN handler --------
        [this, &tls, &cam](const uint8_t *payload, size_t len)
        {
            if (len < 1)
                return;

            const uint8_t t = payload[0];

            if (t == 3) // OPEN
            {
                // Optional future hardening:
                if (!_authed)
                {
                    setError("auth_required");
                    camStop(cam);
                    _wss.disconnect(tls);
                    _state = State::ERROR;
                    return;
                }

                if (!camConnect(cam))
                {
                    setError("camera_connect_failed");
                    camStop(cam);
                    _wss.disconnect(tls);
                    _state = State::ERROR;
                    return;
                }
                _state = State::TUNNELING;
                return;
            }

            if (t == 4) // CLOSE
            {
                camStop(cam);
                _wss.disconnect(tls);
                _state = State::DONE;
                return;
            }

            if (t == 1) // server->camera data
            {
                if (!_authed)
                    return;
                if (_state != State::TUNNELING || !_camConnected)
                    return;
                if (len <= 1)
                    return;

                const uint8_t *rtsp = payload + 1;
                const size_t rtspLen = len - 1;

                // One-time RTSP validation (first request only)
                // we do not want attackers sending bogus RTSP to the camera
                if (!rtspInspectServerToCam_(rtsp, rtspLen))
                {
                    setError("rtsp_rejected");
                    camStop(cam);
                    _wss.disconnect(tls);
                    _state = State::ERROR;
                    return;
                }

                cam.write(payload + 1, len - 1);
                return;
            }
        },

        // -------- TEXT handler (AUTH) --------
        [this, &tls, &cam](const String &msgIn)
        {
            String msg = msgIn;
            msg.trim();

            if (msg.equalsIgnoreCase("READY"))
            {
                _ready = true;
                if (!_helloSent)
                {
                    const String hello = buildHello_();
                    _wss.sendText(tls, hello.c_str());
                    _helloSent = true;
                }
                return;
            }

            if (msg.startsWith("CHAL "))
            {
                _nonceB64 = msg.substring(5);
                _nonceB64.trim();

                const String auth = buildAuth_(_nonceB64);
                if (auth.length() == 0)
                {
                    setError("auth_sign_failed");
                    camStop(cam);
                    _wss.disconnect(tls);
                    _state = State::ERROR;
                    return;
                }

                _wss.sendText(tls, auth.c_str());
                return;
            }

            if (msg.equalsIgnoreCase("AUTH_OK"))
            {
                _authed = true;
                return;
            }

            if (msg.startsWith("AUTH_FAIL"))
            {
                setError("auth_failed");
                camStop(cam);
                _wss.disconnect(tls);
                _state = State::ERROR;
                return;
            }
        });

    if (_state == State::TUNNELING && _authed)
        sendCamToServer(tls, cam);
}

bool WssRtspTunnel::camConnect(Client &cam)
{
    camStop(cam);

    if (!cam.connect(_cfg.camHost.c_str(), _cfg.camPort))
    {
        _camConnected = false;
        return false;
    }

    _camConnected = true;
    return true;
}

void WssRtspTunnel::camStop(Client &cam)
{
    if (cam.connected())
        cam.stop();
    _camConnected = false;
}

void WssRtspTunnel::sendCamToServer(SecureClient &tls, Client &cam)
{
    if (!_camConnected || !cam.connected())
        return;

    const int avail = cam.available();
    if (avail <= 0)
        return;

    uint8_t buf[1024];
    const int n = cam.read(buf, avail > (int)sizeof(buf) ? (int)sizeof(buf) : avail);
    if (n <= 0)
        return;

    uint8_t out[1025];
    out[0] = 2;
    memcpy(out + 1, buf, n);

    _bytesUp += (uint32_t)n;
    _wss.sendBin(tls, out, (size_t)n + 1);
}