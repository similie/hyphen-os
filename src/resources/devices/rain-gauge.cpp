#include "rain-gauge.h"
/**
 * @deconstructor
 */
RainGauge::~RainGauge()
{
}
/**
 * @constructor
 */
RainGauge::RainGauge(Bootstrap *boots)
{
    this->boots = boots;
}

/**
 * @public
 *
 * name
 *
 * Returns the device name
 * @return String
 */
String RainGauge::name()
{
    return this->deviceName;
}

/**
 * @public
 *
 * init
 *
 * Called at setup
 *
 * @return void
 */
void RainGauge::init()
{
    setPin();
    cloudFunctions();
    setInterrupt();
    reqestAddress();
    pullStoredConfig();
}

/**
 * @public
 *
 * read
 *
 * Called during a read event
 *
 * @return void
 */
void RainGauge::read()
{
    // no action required
    print();
}

/**
 * @public
 *
 * publish
 *
 * Called during a publish event
 *
 * @return void
 */
void RainGauge::publish(JsonObject &writer, uint8_t attempt_count)
{
    errorCount = 0;
    double value = counts * perTipMultiple;
    if (isnan(value))
    {
        errorCount = 1;
        value = NO_VALUE;
    }
    counts = 0;
    writer[valueMap[precipitation]] = value;
}

/**
 * @public
 *
 * loop
 *
 * Called during a loop event
 *
 * @return void
 */
void RainGauge::loop()
{
    if (interruptTrpped)
    {
        counts++;
        delay(200);
        interruptTrpped = false;
    }
}

/**
 * @public
 *
 * clear
 *
 * Called during a clear event
 *
 * @return void
 */
void RainGauge::clear()
{
    counts = 0;
}

/**
 * @public
 *
 * print
 *
 * Called during a print event
 *
 * @return void
 */
void RainGauge::print()
{
    Utils::log("RAIN_GAUGE_COUNTS", String(counts));
}

/**
 * @public
 *
 * buffSize
 *
 * Returns the payload size the device requires for sending data
 *
 * @return size_t
 */
size_t RainGauge::buffSize()
{
    return 25;
}

/**
 * @public
 *
 * paramCount
 *
 * Returns the number of params returned
 *
 * @return uint8_t
 */
uint8_t RainGauge::paramCount()
{
    return 1;
}

/**
 * @public
 *
 * maintenanceCount
 *
 * Is the device functional
 *
 * @return uint8_t
 */
uint8_t RainGauge::maintenanceCount()
{
    return errorCount;
}

/**
 * @public
 *
 * restoreDefaults
 *
 * Restores the default value setup in DEFAULT_TIP_SIZE
 * @return String
 */
void RainGauge::restoreDefaults()
{
    perTipMultiple = DEFAULT_TIP_SIZE;
    if (!validAddress())
    {
        return;
    }
    config = {1, perTipMultiple};
    Persist.put(eepromAddress, config);
}

///////////////////////
// Privates
///////////////////////

/**
 * @private
 *
 * validAddress
 *
 * Checks to see that the eepromAddress suppled is valid
 */
bool RainGauge::validAddress()
{
    return boots->doesNotExceedsMaxAddressSize(eepromAddress);
}

/**
 * @private
 *
 * setTipMultiple
 *
 * Cloud callback function for setting the tipMultiple value
 *
 * @param String value - send from the cloud
 *
 * @return int
 */
int RainGauge::setTipMultiple(String value)
{
    double val = Utils::parseCloudFunctionDouble(value, name());

    if (val == 0)
    {
        return 0;
    }
    if (validAddress())
    {
        config = {1, val};
        Persist.put(eepromAddress, config);
    }

    perTipMultiple = val;
    return 1;
}
/**
 * @private
 *
 * cloudFunctions
 *
 * Registers the cloud functions
 *
 * @return void
 */
void RainGauge::cloudFunctions()
{
    // Hyphen.function("setTipMultiple", &RainGauge::setTipMultiple, this);
    // Hyphen.variable(String(name() + "TipMultiple").c_str(), perTipMultiple);
    // Hyphen.variable(String(name() + "Count").c_str(), counts);
}
/**
 * @private
 *
 * countChange
 *
 * Interrupt callback for a tip change
 *
 * @return void
 */
void RainGauge::countChange()
{
    this->interruptTrpped = true;
}
/**
 * @private
 *
 * setInterrupt
 *
 * Sets the interrupt
 *
 * @return void
 */
void RainGauge::setInterrupt()
{
    // attachInterrupt(RAIN_GAUGE_PIN, &RainGauge::countChange, this, RISING);
}
/**
 * @private
 *
 * setPin
 *
 * Sets the listening pin in the RAIN_GAUGE_PIN variable
 *
 * @return void
 */
void RainGauge::setPin()
{
    // pinMode(RAIN_GAUGE_PIN, INPUT_PULLDOWN);
}

/**
 * @private
 *
 * setPerTipMultiple
 *
 * Sets the perTipMultiple based on the stored device config
 *
 * @return void
 */
void RainGauge::setPerTipMultiple()
{
    Utils::log("CONFIGURATION_PULLED_FOR " + name(), "value: " + String(config.version) + " " + String(config.calibration));
    if (Utils::validConfigIdentity(config.version))
    {
        perTipMultiple = config.calibration;
    }
}
/**
 * @private
 *
 * pullStoredConfig
 *
 * Pulls the stored config from EEPROM
 *
 * @return void
 */
void RainGauge::pullStoredConfig()
{
    Utils::log("ADDRESS_PULLED FOR " + name(), "value: " + String(eepromAddress));
    if (validAddress())
    {
        Persist.get(eepromAddress, config);
        setPerTipMultiple();
    }
}
/**
 * @private
 *
 * reqestAddress
 *
 * Gets the EEPROM Address
 *
 * @return void
 */
void RainGauge::reqestAddress()
{
    eepromAddress = boots->registerAddress(name(), sizeof(RainGaugeStruct));
}
