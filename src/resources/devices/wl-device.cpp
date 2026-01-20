#include "wl-device.h"

static uint32_t absDiffU32(uint32_t a, uint32_t b)
{
    return (a > b) ? (a - b) : (b - a);
}

static uint32_t medianOfSorted(const uint32_t *v, size_t n)
{
    return v[n / 2];
}

WlDevice::~WlDevice() {}

WlDevice::WlDevice(Bootstrap *boots)
{
    this->boots = boots;
}

WlDevice::WlDevice(Bootstrap *boots, int sendIdentity)
{
    this->boots = boots;
    this->sendIdentity = sendIdentity;
}

WlDevice::WlDevice(Bootstrap *boots, int sendIdentity, int readPin)
{
    this->boots = boots;
    this->sendIdentity = sendIdentity;
    this->readPin = readPin;
}

String WlDevice::name()
{
    return this->deviceName;
}

int WlDevice::getPin()
{
    if (this->readPin != -1)
        return this->readPin;

    return digital ? (int)DIG_PIN : (int)AN_PIN;
}

void WlDevice::setPinMode()
{
    const int pin = getPin();

    if (digital)
    {
        // If your sensor is open-drain, use INPUT_PULLUP instead.
        pinMode(pin, INPUT);
    }
    else
    {
        pinMode(pin, INPUT);
    }

    // lock in resolved pin so later changes to `digital` don’t surprise you
    this->readPin = pin;
}

void WlDevice::saveEEPROM(WLStruct storage)
{
    if (saveAddressForWL == 0)
        return;

    storage.version = 1;
    Persist.put(saveAddressForWL, storage);
}

WLStruct WlDevice::getProm()
{
    WLStruct prom{};
    Persist.get(saveAddressForWL, prom);
    return prom;
}

char WlDevice::setDigital(bool value)
{
    return value ? 'y' : 'n';
}

bool WlDevice::isDigital(char value)
{
    if (value == 'y')
        return true;
    if (value == 'n')
        return false;
    return DIGITAL_DEFAULT;
}

bool WlDevice::isSaneCalibration(double cal, bool digitalMode)
{
    if (!isfinite(cal))
        return false;

    if (digitalMode)
    {
        // typical: 1/58 ≈ 0.01724
        return cal > 0.001 && cal < 0.1;
    }
    else
    {
        return cal > 0.0001 && cal < 10.0;
    }
}

bool WlDevice::hasSerialIdentity()
{
    return utils.hasSerialIdentity(this->sendIdentity);
}

String WlDevice::appendIdentity()
{
    return this->hasSerialIdentity() ? String(this->sendIdentity) : "";
}

String WlDevice::uniqueName()
{
    if (this->hasSerialIdentity())
        return this->name() + String(this->sendIdentity);
    return this->name();
}

void WlDevice::setCloudFunctions()
{
    String appendage = appendIdentity();
    Hyphen.function("setDistanceAsDigital" + appendage, &WlDevice::setDigitalCloud, this);
    Hyphen.function("setDistanceCalibration" + appendage, &WlDevice::setCalibration, this);
    Hyphen.variable("isDistanceDigital" + appendage, &digital);
    Hyphen.variable("getDistanceCalibration" + appendage, &currentCalibration);
}

int WlDevice::setDigitalCloud(String read)
{
    int val = (int)atoi(read.c_str());
    if (val < 0 || val > 1)
        return 0;

    digital = (bool)val;

    WLStruct storage = getProm();
    storage.digital = setDigital(digital);

    // keep existing calibration if migrated/invalid
    if (!Utils::validConfigIdentity(storage.version))
        storage.calibration = currentCalibration;

    saveEEPROM(storage);

    // apply immediately
    setPinMode();

    return 1;
}

int WlDevice::setCalibration(String read)
{
    double val = Utils::parseCloudFunctionDouble(read, uniqueName());
    if (val == 0)
        return 0;

    currentCalibration = val;

    WLStruct storage = getProm();
    storage.calibration = currentCalibration;

    if (!Utils::validConfigIdentity(storage.version))
        storage.digital = setDigital(digital);

    saveEEPROM(storage);
    return 1;
}

void WlDevice::restoreDefaults()
{
    digital = DIGITAL_DEFAULT;
    currentCalibration = digital ? DEF_DISTANCE_READ_DIG_CALIBRATION : DEF_DISTANCE_READ_AN_CALIBRATION;

    config.calibration = currentCalibration;
    config.digital = setDigital(digital);

    saveEEPROM(config);
    setPinMode();
}

void WlDevice::configSetup()
{
    digital = isDigital(this->config.digital);

    double cal = this->config.calibration;
    if (!isSaneCalibration(cal, digital))
    {
        restoreDefaults();
        cal = this->config.calibration;
    }

    currentCalibration = cal;
    setPinMode();
}

uint32_t WlDevice::readPulseUs(uint32_t timeoutUs)
{
    // Single-source-of-truth digital read:
    // pulseIn returns 0 on timeout (no pulse seen)

    const int pin = getPin();
    const uint32_t start = micros();

    // 1) wait for any current HIGH to end
    while (digitalRead(pin) == HIGH)
    {
        if ((micros() - start) > timeoutUs)
            return 0;
    }

    return (uint32_t)pulseIn(pin, HIGH, timeoutUs);
}

uint32_t WlDevice::getReadValue()
{
    if (digital)
    {
        constexpr uint32_t timeoutUs = 200000;
        return readPulseUs(timeoutUs);
    }
    else
    {
        return (uint32_t)analogRead(getPin());
    }
}
uint32_t WlDevice::readWL()
{
    constexpr size_t targetSamples = 7;    // was 3
    constexpr uint32_t timeoutUs = 200000; // match getReadValue
    const uint32_t minUs = 800;
    const uint32_t maxUs = 60000; // your clamp

    uint32_t reads[targetSamples];
    size_t count = 0;

    uint32_t startMs = millis();
    while (count < targetSamples && (millis() - startMs) < 900)
    {
        uint32_t v = getReadValue();

        if (digital)
        {
            if (v == 0)
            {
                delay(8);
                continue;
            }
            if (v < minUs || v > maxUs)
            {
                delay(8);
                continue;
            }
        }

        reads[count++] = v;
        delay(18);
    }

    if (count == 0)
        return 0;

    // sort reads[0..count)
    for (size_t i = 0; i < count; i++)
        for (size_t j = i + 1; j < count; j++)
            if (reads[j] < reads[i])
            {
                uint32_t t = reads[i];
                reads[i] = reads[j];
                reads[j] = t;
            }

    if (!digital)
    {
        double scaled = (double)reads[count / 2] * currentCalibration;
        return (uint32_t)lround(scaled);
    }

    // ---- Cluster selection for digital pulses ----
    // Group pulses that are within `tolUs` of each other.
    constexpr uint32_t tolUs = 180; // adjust 120–250 depending on noise

    // Find best cluster by counting membership around each candidate.
    size_t bestIdx = 0;
    size_t bestCount = 0;

    for (size_t i = 0; i < count; i++)
    {
        size_t c = 1;
        for (size_t j = 0; j < count; j++)
        {
            if (i == j)
                continue;
            if (absDiffU32(reads[i], reads[j]) <= tolUs)
                c++;
        }

        if (c > bestCount)
        {
            bestCount = c;
            bestIdx = i;
        }
        else if (c == bestCount && lastGoodPwUs != 0)
        {
            // tie-break: choose closer to last good
            if (absDiffU32(reads[i], lastGoodPwUs) < absDiffU32(reads[bestIdx], lastGoodPwUs))
                bestIdx = i;
        }
    }

    // Collect members of the winning cluster into tmp
    uint32_t tmp[targetSamples];
    size_t k = 0;
    for (size_t i = 0; i < count; i++)
        if (absDiffU32(reads[i], reads[bestIdx]) <= tolUs)
            tmp[k++] = reads[i];

    // tmp is already mostly sorted because reads is sorted, but keep it safe:
    for (size_t i = 0; i < k; i++)
        for (size_t j = i + 1; j < k; j++)
            if (tmp[j] < tmp[i])
            {
                uint32_t t = tmp[i];
                tmp[i] = tmp[j];
                tmp[j] = t;
            }

    uint32_t pw = medianOfSorted(tmp, k);
    lastGoodPwUs = pw;

    double cm = (double)pw * currentCalibration;
    Serial.printf("pw_us=%lu cm=%.2f cal=%.10f cluster=%u/%u\n",
                  (unsigned long)pw, cm, currentCalibration, (unsigned)k, (unsigned)count);

    return (uint32_t)lround(cm);
}

String WlDevice::getParamName(size_t index)
{
    String param = readParams[index];
    if (this->hasSerialIdentity())
        return param + StringFormat("_%d", sendIdentity);
    return param;
}

void WlDevice::publish(JsonObject &writer, uint8_t attempt_count, const String &payloadId)
{
    for (size_t i = 0; i < PARAM_LENGTH; i++)
    {
        String param = getParamName(i);
        int median = utils.getMedian(attempt_count, VALUE_HOLD[i]);

        if (median == 0)
            maintenanceTick++;

        writer[utils.stringConvert(param)] = median;
        Log.info("Param=%s has median %d", utils.stringConvert(param), median);
    }
}

void WlDevice::init()
{
    setCloudFunctions();

    Utils::log("WL_BOOT_ADDRESS REGISTRATION", this->uniqueName());
    saveAddressForWL = boots->registerAddress(this->uniqueName(), sizeof(WLStruct));
    Utils::log("WL_BOOT_ADDRESS " + this->uniqueName(), String(saveAddressForWL));

    this->config = getProm();
    if (!Utils::validConfigIdentity(this->config.version))
    {
        restoreDefaults();
        this->config = getProm(); // reload after restore
    }

    configSetup();
}

void WlDevice::read()
{
    uint32_t cm = readWL();
    Serial.printf("WL cm=%d\n", (int)cm);
    Log.info("WL %d", (int)cm);

    for (size_t i = 0; i < PARAM_LENGTH; i++)
    {
        utils.insertValue((int)cm, VALUE_HOLD[i], boots->getMaxVal());
    }
}

void WlDevice::loop() {}

void WlDevice::clear()
{
    for (size_t i = 0; i < PARAM_LENGTH; i++)
    {
        for (size_t j = 0; j < boots->getMaxVal(); j++)
        {
            VALUE_HOLD[i][j] = NO_VALUE;
        }
    }
}

void WlDevice::print()
{
    for (size_t i = 0; i < PARAM_LENGTH; i++)
    {
        for (size_t j = 0; j < boots->getMaxVal(); j++)
        {
            Log.info("PARAM VALUES FOR %s of iteration %d and value %d",
                     utils.stringConvert(readParams[i]), (int)j, (int)VALUE_HOLD[i][j]);
        }
    }
}

size_t WlDevice::buffSize()
{
    return 40;
}

uint8_t WlDevice::paramCount()
{
    return (uint8_t)PARAM_LENGTH;
}

uint8_t WlDevice::maintenanceCount()
{
    uint8_t maintenance = this->maintenanceTick;
    maintenanceTick = 0;
    return maintenance;
}
