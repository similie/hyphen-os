#include "flow-meter.h"

FlowMeter::~FlowMeter()
{
}

FlowMeter::FlowMeter()
{
    pulseCount = 0;
}

FlowMeter::FlowMeter(Bootstrap *boots)
{
    pulseCount = 0;
    this->boots = boots;
    clear();
}

FlowMeter::FlowMeter(Bootstrap *boots, int sendIdentity)
{
    pulseCount = 0;
    this->boots = boots;
    this->sendIdentity = sendIdentity;
    clear();
}

FlowMeter::FlowMeter(Bootstrap *boots, int sendIdentity, int readPin)
{
    pulseCount = 0;
    this->flowPin = readPin;
    this->boots = boots;
    this->sendIdentity = sendIdentity;
    clear();
}

String FlowMeter::name()
{
    return this->deviceName;
}

void FlowMeter::restoreDefaults()
{
}

void FlowMeter::init()
{
    instantated = true;
    setDeviceAddress();
    setLifeTimeFlow();
    setConfiguration();
    setListeners();
    configurePin();
}

void FlowMeter::publish(JsonObject &writer, uint8_t attempt_count)
{
    // removeInterrupt();
    for (size_t i = 0; i < PARAM_LENGTH; i++)
    {
        String param = getParamName(i);
        switch (i)
        {
        case c_flow:
            writer[utils.stringConvert(param)] = currentFlow;
            break;
        case t_flow:
            writer[utils.stringConvert(param)] = totalMilliLitres;
            break;
        }
    }
    setFlow();
}

void FlowMeter::read()
{
}

void FlowMeter::loop()
{
    processRead();
}

void FlowMeter::clear()
{
    currentFlow = 0;
    // setInterrupt();
}

void FlowMeter::print()
{
    // Print the flow rate for this second in litres / minute

    Serial.print("Flow rate: ");
    Serial.print(flowRate); // Print the integer part of the variable
    Serial.print("L/min");
    Serial.print("\t"); // Print tab space

    // Print the cumulative total of litres flowed since starting
    Serial.print("Output Liquid Quantity: ");
    Serial.print(currentFlow);
    Serial.print("mL");

    Serial.print("\t"); // Print tab space
    Serial.print(totalMilliLitres);
    Serial.print("mL");

    Serial.print("\t"); // Print tab space
    Serial.print(totalMilliLitres / 1000);
    Serial.println("L");
}

size_t FlowMeter::buffSize()
{
    return 60;
}

uint8_t FlowMeter::paramCount()
{
    return 4;
}

uint8_t FlowMeter::maintenanceCount()
{
    return 0;
}

void FlowMeter::setPin(int pin)
{
    flowPin = pin;
}

void FlowMeter::configurePin()
{
    pinMode(getPin(), INPUT);
    digitalWrite(getPin(), HIGH);
    setInterrupt();
}

///////////////////////////
/// Privates
///////////////////////////

void FlowMeter::setConfiguration()
{
    FlowStruct storage = getProm();
    if (!Utils::validConfigIdentity(storage.version))
    {
        return;
    }

    if (storage.calibrationDifference != 0 && storage.calibrationDifference < 200 && !isnan(storage.calibrationDifference))
    {
        calibrationDifference = storage.calibrationDifference;
    }

    if (storage.calibrationFactor != 0 && storage.calibrationFactor < 200 && !isnan(storage.calibrationFactor))
    {
        calibrationFactor = storage.calibrationFactor;
    }
}

/**
 * @private
 *
 * getPin
 *
 * Gets the configured pin
 *
 * @return int
 */
int FlowMeter::getPin()
{
    return this->flowPin;
}

void FlowMeter::setLifeTimeFlow()
{
    totalMilliLitres = getToltalFlowMiliLiters();
}

/**
 *
 *
 */
void FlowMeter::setInterrupt()
{
    if (!instantated)
    {
        return;
    }
    attachInterruptArg(getPin(), pulseCounter, this, FALLING);
}

bool FlowMeter::isDisconnected()
{
    return pulseCount >= 12;
}

void FlowMeter::processRead()
{

    if ((millis() - lastTime) > 1000) // Only process counters once per second
    {
        if (isDisconnected())
        {
            lastTime = millis();
            String append = appendIdentity();
            Utils::log("FLOW_METER_DISCONNECT", append + " " + String(pulseCount));
            pulseCount = 0;
            return;
        }

        removeInterrupt();
        // Because this loop may not complete in exactly 1 second intervals we calculate
        // the number of milliseconds that have passed since the last execution and use
        // that to scale the output. We also apply the calibrationFactor to scale the output
        // based on the number of pulses per second per units of measure (litres/minute in
        // this case) coming from the sensor.
        float tmp = (1000.0 / (millis() - lastTime)) * pulseCount;
        countIteration++;
        Serial.print("ITERATION: ");
        Serial.print(countIteration); // Print the integer part of the variable
        Serial.print(" ");
        Serial.print("\t"); // Print tab space

        Serial.print("PULSE: ");
        Serial.print(pulseCount); // Print the integer part of the variable
        Serial.print(" ");
        Serial.print("\t"); // Print tab space

        Serial.print("TMP: ");
        Serial.print(tmp); // Print the integer part of the variable
        Serial.print(" ");
        Serial.print("\t"); // Print tab space

        // As we have a add operation we need to avoid the positive value when no pulse is found
        if (tmp > 0)
        {
            flowRate = tmp / calibrationFactor + calibrationDifference;
        }
        else
        {
            flowRate = 0;
        }

        // Note the time this processing pass was executed. Note that because we've
        // disabled interrupts the millis() function won't actually be incrementing right
        // at this point, but it will still return the value it was set to just before
        // interrupts went away.
        lastTime = millis();
        // Divide the flow rate in litres/minute by 60 to determine how many litres have
        // passed through the sensor in this 1 second interval, then multiply by 1000 to
        // convert to millilitres.
        flowMilliLitres = (flowRate / 60) * 1000;

        currentFlow += flowMilliLitres;
        // Add the millilitres passed in this second to the cumulative total
        totalMilliLitres += flowMilliLitres;

        print();
        // this serial print was used to get the FREQUENCY value to calculate the other values.
        // Serial.print("F: ");
        // Serial.println(tmp);
        // Reset the pulse counter so we can start incrementing again
        pulseCount = 0;

        setInterrupt();
    }
}

void FlowMeter::removeInterrupt()
{
    detachInterrupt(getPin());
}

/**
 * @private
 *
 * getProm
 *
 * Returns the wl configuration structure
 *
 * @return WlDevice
 */
FlowStruct FlowMeter::getProm()
{
    FlowStruct prom;
    if (saveAddressForFlow == -1)
    {
        return prom;
    }
    Persist.get(saveAddressForFlow, prom);
    return prom;
}

int FlowMeter::clearTotalCount(String val)
{
    if (!val.equals("DELETE"))
    {
        return 0;
    }
    FlowStruct flow = getProm();
    flow.totalMilliLitres = 0;
    saveConfig(flow);
    totalMilliLitres = 0;
    return 1;
}
/**
 * @private
 *
 * setCalibration
 *
 * Cloud function for setting the calibration value
 * @param String read - payload from the particle API
 *
 * @return int
 */
int FlowMeter::setCalibrationFactor(String read)
{
    double val = Utils::parseCloudFunctionDouble(read, uniqueName());
    if (val == 0)
    {
        return 0;
    }

    calibrationFactor = val;

    FlowStruct storage = getProm();
    storage.calibrationFactor = calibrationFactor;
    saveConfig(storage);
    return 1;
}
/**
 * @private
 *
 * setCalibration
 *
 * Cloud function for setting the calibration value
 * @param String read - payload from the particle API
 *
 * @return int
 */
int FlowMeter::setCalibrationDifference(String read)
{
    double val = Utils::parseCloudFunctionDouble(read, uniqueName());
    if (val == 0)
    {
        return 0;
    }

    calibrationDifference = val;

    FlowStruct storage = getProm();
    storage.calibrationDifference = calibrationDifference;
    saveConfig(storage);
    return 1;
}

/**
 * @private
 *
 * saveConfig
 *
 * @param WLStruct storage - the config payload needing storage
 *
 * @return void
 */
void FlowMeter::saveConfig(FlowStruct storage)
{
    if (saveAddressForFlow == -1)
    {
        return;
    }
    storage.version = 1;
    Persist.put(saveAddressForFlow, storage);
}

void FlowMeter::setFlow()
{
    FlowStruct storage = getProm();
    if (storage.totalMilliLitres == totalMilliLitres)
    {
        return;
    }
    storage.totalMilliLitres = totalMilliLitres;
    storage.calibrationDifference = calibrationDifference;
    storage.calibrationFactor = calibrationFactor;
    saveConfig(storage);
}

unsigned long FlowMeter::getToltalFlowMiliLiters()
{
    FlowStruct storage = getProm();
    if (!Utils::validConfigIdentity(storage.version))
    {
        return 0;
    }
    return storage.totalMilliLitres;
}

void FlowMeter::setListeners()
{
    String appendage = appendIdentity();
    Hyphen.function("setCalibrationFactor" + appendage, &FlowMeter::setCalibrationFactor, this);
    Hyphen.function("setCalibrationDifference" + appendage, &FlowMeter::setCalibrationDifference, this);
    Hyphen.function("clearTotalFlow" + appendage, &FlowMeter::clearTotalCount, this);

    Hyphen.variable("getCalibrationFactor" + appendage, &calibrationFactor);
    Hyphen.variable("getCalibrationDifference" + appendage, &calibrationDifference);
}

void FlowMeter::setIdentity(int identity)
{
    sendIdentity = identity;
}

/**
 * @private
 *
 * appendIdentity
 *
 * If there is an integer identity, it concatenates the id as a string
 *
 * @return String
 */
String FlowMeter::appendIdentity()
{
    return this->hasSerialIdentity() ? String(this->sendIdentity) : "";
}

/**
 * @private
 *
 * uniqueName
 *
 * If there is an integer identity, it concats the id with the name, otherwise
 * it simply returns the given name
 *
 * @return String
 */
String FlowMeter::uniqueName()
{
    if (this->hasSerialIdentity())
    {
        return this->name() + String(this->sendIdentity);
    }
    return this->name();
}

String FlowMeter::getParamName(size_t index)
{
    String param = valueMap[index];
    if (this->hasSerialIdentity())
    {
        return param + StringFormat("_%d", sendIdentity);
    }
    return param;
}

/**
 * @private
 *
 * restoreDefaults
 *
 * Restores the default values
 *
 * @return void
 */
bool FlowMeter::hasSerialIdentity()
{
    return utils.hasSerialIdentity(this->sendIdentity);
}

void FlowMeter::setDeviceAddress()
{
    saveAddressForFlow = boots->registerAddress(this->uniqueName(), sizeof(FlowStruct));
    Utils::log("FLOW_METER_BOOTSTRAP_ADDRESS", String(saveAddressForFlow));
}

/**
 *
 *
 */

void FlowMeter::pulseCounter()
{
    pulseCount++;
}

void IRAM_ATTR FlowMeter::pulseCounter(void *arg)
{
    FlowMeter *instance = static_cast<FlowMeter *>(arg);
    if (instance == nullptr)
    {
        return;
    }
    instance->pulseCounter();
}