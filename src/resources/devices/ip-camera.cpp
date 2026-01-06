#include "ip-camera.h"
#include "Hyphen.h"

// ---------- ctor/dtor ----------
IPCamera::IPCamera()
    : _sockCfg(SocketConfig::fromBuildFlags()),
      _tunnel(_sockCfg) {}

IPCamera::IPCamera(Bootstrap *boots)
    : _boots(boots),
      _sockCfg(SocketConfig::fromBuildFlags()),
      _tunnel(_sockCfg) {}

IPCamera::~IPCamera() {}

// ---------- basics ----------
String IPCamera::name() { return "IPCamera"; }

void IPCamera::cloudFunctions()
{
    Hyphen.function(String(name() + "setSendOffset").c_str(), &IPCamera::setOffsetMultiple, this);
    Hyphen.variable(String(name() + "sendOffset").c_str(), &_offset);

    Hyphen.function(String(name() + "snapNow").c_str(), &IPCamera::snapNow, this);
}

void IPCamera::requestAddress()
{
    if (_boots)
        _eepromAddress = _boots->registerAddress(name(), sizeof(IPCameraConfigStruct));
}

bool IPCamera::validAddress()
{
    if (!_boots)
        return false;
    return _boots->doesNotExceedsMaxAddressSize(_eepromAddress);
}

void IPCamera::pullStoredConfig()
{
    if (!_boots)
        return;
    if (validAddress())
    {
        Persist.get(_eepromAddress, _config);
        setOffsetValue();
    }
}

void IPCamera::saveOffset(uint8_t offset)
{
    if (!_boots)
        return;
    if (!validAddress())
        return;

    _config = {1, offset};
    Persist.put(_eepromAddress, _config);
}

void IPCamera::setOffsetValue()
{
    if (Utils::validConfigIdentity(_config.version))
    {
        _offset = (int)_config.offset;
        if (_offset <= 0)
            _offset = 1;
        _offsetCount = 0;
    }
}

bool IPCamera::readySend()
{
    _offsetCount++;
    const uint8_t o = (uint8_t)(_offset <= 0 ? 1 : _offset);
    return (_offsetCount % o) == 0;
}

// ---------- init ----------
void IPCamera::init()
{
    cloudFunctions();
    requestAddress();

#ifdef HYPHEN_IPCAM_OFFSET_DEFAULT
    if (_offset <= 0)
        _offset = (int)HYPHEN_IPCAM_OFFSET_DEFAULT;
#endif

    pullStoredConfig();
    _tunnel.begin();

    // queue: size 3 is plenty (forced + periodic + one extra)
    _q = xQueueCreate(3, sizeof(SnapJob));

    // worker task
    xTaskCreatePinnedToCore(
        &IPCamera::taskThunk,
        "IPCamSnap",
        8192, // stack (tune if needed)
        this,
        1, // priority (keep low)
        &_task,
        1 // core 1 typically; adjust if HyphenOS uses core 1 heavily
    );
    _taskRunning = (_task != nullptr);

    Utils::log("IPCAM_INIT",
               "cam=" + _sockCfg.camHost + ":" + String(_sockCfg.camPort) +
                   " wss=" + _sockCfg.wssHost + ":" + String(_sockCfg.wssPort) + _sockCfg.wssPath);
}

// ---------- cloud callbacks ----------
int IPCamera::setOffsetMultiple(String value)
{
    const int val = Utils::parseCloudFunctionInteger(value, name());
    if (val <= 0)
        return 0;

    _offset = val;
    _offsetCount = 0;
    saveOffset((uint8_t)val);
    return val;
}

int IPCamera::snapNow(String value)
{
    (void)value;
    _snapRequested = true;
    return 1;
}

// ---------- enqueue ----------
void IPCamera::enqueueSnap(uint32_t timeoutMs, bool forced, const String &payloadId)
{
    if (!_q)
        return;

    const uint32_t now = millis();
    if (now < _nextAllowedStart)
    {
        // still backing off (server down / repeated fails)
        return;
    }

    SnapJob j{};
    j.timeoutMs = timeoutMs;
    j.forced = forced;

    // copy payloadId safely into fixed buffer
    j.payloadId[0] = '\0';
    if (payloadId.length() > 0)
    {
        strncpy(j.payloadId, payloadId.c_str(), sizeof(j.payloadId) - 1);
        j.payloadId[sizeof(j.payloadId) - 1] = '\0';
    }

    // non-blocking: if queue full, drop the request (forced will try again next loop if still requested)
    (void)xQueueSend(_q, &j, 0);
}

// ---------- publish/loop ----------
// publish = cadence scheduler only (do not run work here)
void IPCamera::publish(JsonObject &writer, uint8_t attempt_count, const String &payloadId)
{
    (void)writer;
    (void)attempt_count;

    if (readySend())
    {
        _offsetCount = 0;

        uint32_t timeout =
#ifdef HYPHEN_IPCAM_TUNNEL_TIMEOUT_MS
            (uint32_t)HYPHEN_IPCAM_TUNNEL_TIMEOUT_MS;
#else
            45000;
#endif
        // Serial.println("IPCAM: Enqueue snap with payloadId: " + payloadId);
        enqueueSnap(timeout, false, payloadId);
    }
}

// loop = immediate triggers (snapNow) only
void IPCamera::loop()
{
    if (_snapRequested)
    {
        _snapRequested = false;

        uint32_t timeout =
#ifdef HYPHEN_IPCAM_TUNNEL_TIMEOUT_MS
            (uint32_t)HYPHEN_IPCAM_TUNNEL_TIMEOUT_MS;
#else
            45000;
#endif
        // String payloadId = DeviceSecurity::makePayloadId(Hyphen.deviceID());
        enqueueSnap(timeout, true, "");
    }

    // IMPORTANT: do nothing else heavy here; let HyphenOS keep petting watchdog
}

void IPCamera::read() {}

// ---------- FreeRTOS task ----------
void IPCamera::taskThunk(void *arg)
{
    ((IPCamera *)arg)->taskMain();
}

void IPCamera::taskMain()
{
    for (;;)
    {
        SnapJob job{};
        if (!_q)
        {
            vTaskDelay(pdMS_TO_TICKS(250));
            continue;
        }

        // wait forever for work
        if (xQueueReceive(_q, &job, portMAX_DELAY) != pdTRUE)
            continue;

        // (optional) if HyphenOS has a task-safe watchdog pet, you can call it here too

        // Start the snapshot window using independent clients
        Connection &conn = Hyphen.hyConnect().getConnection();
        Client &cam = conn.getNewClient();
        SecureClient &tls = conn.getNewSecureClient();
        String payloadId = String(job.payloadId); // safe
        const uint32_t start = millis();
        bool started = _tunnel.startSnapshot(tls, cam, payloadId);
        if (!started)
        {
            // backoff on immediate connect failure
            _nextAllowedStart = millis() + _backoffMs;
            _backoffMs = (_backoffMs < _backoffMaxMs) ? min(_backoffMs * 2, _backoffMaxMs) : _backoffMaxMs;
            Utils::log("IPCAM_SNAP_FAIL", String(_tunnel.lastError()));
            continue;
        }

        // Pump until finished or timeout; yield so OS stays happy
        while (!_tunnel.finished() && (millis() - start < job.timeoutMs))
        {
            _tunnel.loop(tls, cam);
            vTaskDelay(pdMS_TO_TICKS(2));
        }

        // Stop is idempotent; don’t clobber DONE/ERROR
        _tunnel.stop(tls, cam);

        const bool ok = _tunnel.isOk();

        if (ok)
        {
            _backoffMs = 1000;
            _nextAllowedStart = 0;
            Utils::log("IPCAM_SNAP_OK", "done");
        }
        else
        {
            _nextAllowedStart = millis() + _backoffMs;
            _backoffMs = (_backoffMs < _backoffMaxMs) ? min(_backoffMs * 2, _backoffMaxMs) : _backoffMaxMs;
            Utils::log("IPCAM_SNAP_FAIL", String(_tunnel.lastError()));
        }
    }
}

// ---------- boilerplate ----------
uint8_t IPCamera::maintenanceCount() { return 0; }
uint8_t IPCamera::paramCount() { return 0; }
void IPCamera::clear() {}
void IPCamera::print() {}
size_t IPCamera::buffSize() { return 0; }
void IPCamera::restoreDefaults() {}