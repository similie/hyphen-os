#include "video-capture.h"
/**
 *
 * This is the primary api for the VideoCapture classes. All public attributes must be inherited by the
 * VideoCapture object, even of they provide no operational function to the working of the object. Just
 * leave the function blank as below.
 *
 */

/**
 * @deconstructor
 */
VideoCapture::~VideoCapture()
{
}

/**
 * @constructor
 */
VideoCapture::VideoCapture(Bootstrap *boots)
{
    this->boots = boots;
}

/**
 * @constructor
 */
VideoCapture::VideoCapture()
{
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
void VideoCapture::publish(JsonObject &writer, uint8_t attempt_count, const String &payloadId)
{

    if (readySend())
    {
        takeShot();
        offsetCount = 0;
    }
}

/**
 * @public
 *
 * restoreDefaults
 *
 * Called when defaults should be restored
 *
 * @return void
 */
void VideoCapture::restoreDefaults()
{
}

/**
 * @public
 *
 * name
 *
 * Returns the VideoCapture name
 * @return String
 */
String VideoCapture::name()
{
    return "VideoCapture";
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
uint8_t VideoCapture::paramCount()
{
    return 0;
}

/**
 * @public
 *
 * maintenanceCount
 *
 * Is the VideoCapture functional
 *
 * @return uint8_t
 */
uint8_t VideoCapture::maintenanceCount()
{
    return 0;
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
void VideoCapture::read()
{
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
void VideoCapture::loop()
{
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
void VideoCapture::clear()
{
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
void VideoCapture::print()
{
}

void VideoCapture::cloudFunctions()
{
    Hyphen.function(String(name() + "SetSendOffset").c_str(), &VideoCapture::setOffsetMultiple, this);
    Hyphen.variable(String(name() + "SendOffset").c_str(), &offset);
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
void VideoCapture::init()
{
    cloudFunctions();
    requestAddress();
    pullStoredConfig();
    relay.init();
}
/**
 * @public
 *
 * buffSize
 *
 * Returns the payload size the VideoCapture requires for sending data
 *
 * @return size_t
 */
size_t VideoCapture::buffSize()
{
    return 0;
}

bool VideoCapture::readySend()
{
    offsetCount++;
    return offsetCount % (u_int8_t)offset == 0;
}

char *VideoCapture::getVersion()
{
    cam.begin();
    delay(200);

    for (u_int8_t i = 0; i < VERSION_CHECK; i++)
    {
        char *checkedVersion = cam.getVersion();
        Serial.print("CHECKING VERSION");
        Serial.println(checkedVersion);
        if (checkedVersion != 0)
        {
            return checkedVersion;
        }
        delay(200);
    }
    return 0;
}

bool VideoCapture::setupCamera()
{
    // delay(5000);
    cam.setImageSize(VC0706_640x480);
    // Print out the camera version information (optional)
    delay(200);
    version = getVersion(); // cam.getVersion();
    if (version == 0)
    {
        Utils::log("CAMERA_ACTIVATION_FALURE " + name(), "Failed to get version");
        cameraReady = false;
    }
    else
    {
        Utils::log("CAMERA_ACTIVATION " + name(), String(version));
        cameraReady = true;
    }
    return cameraReady;
}

bool VideoCapture::isConnected()
{
    return client.status();
}

bool VideoCapture::connnectToServer()
{
    connected = client.connect(server, port);
    return connected;
}

bool VideoCapture::shotStartUp()
{
    bool connected = false;
    u_int16_t timeout = 10000;
    unsigned long start = millis();
    unsigned long delta = 0;
    do
    {
        connected = connnectToServer();
        delta = millis() - start;
    } while (!connected && delta < timeout);

    return connected;
}

bool VideoCapture::takeShot()
{

    if (!on())
    {
        return off();
    }

    if (!shotStartUp())
    {
        return off();
    }

    if (!snapPhoto())
    {
        return off();
    }

    if (!transmitImageData())
    {
        return off();
    }

    disconnect();
    reset();

    return !off();
}

bool VideoCapture::on()
{
    relay.on();
    delay(1000);
    cam.begin();
    delay(200);
    return setupCamera();
}

bool VideoCapture::off()
{
    cam.end();
    return relay.off();
}

bool VideoCapture::snapPhoto()
{
    preCaptureCamera();
    delay(200);
    bool taken = cam.takePicture();
    return taken;
}

bool VideoCapture::transmitImageData()
{
    writeHead();
    uint16_t size = writeBuffer();
    writeClose();
    if (!size)
    {
        return false;
    }

    return true;
}

void VideoCapture::preCaptureCamera()
{
    cam.setMotionDetect(false);
    cam.setCompression(99);
}

void VideoCapture::sendBuffer(uint8_t *buffer, uint8_t bytesToRead)
{
    client.write(buffer, bytesToRead);
}

void VideoCapture::writeAString(String topic)
{
    String send = String(topic);
    unsigned int length = send.length() + 1;
    byte buff[length];
    send.getBytes(buff, length);
    if (isConnected())
    {
        client.write(buff, length);
    }
}

void VideoCapture::reset()
{
    cam.reset();
}

void VideoCapture::disconnect()
{
    client.stop();
}

uint16_t VideoCapture::writeBuffer()
{

    if (!isConnected())
    {
        return 0;
    }

    uint16_t jpglen = cam.frameLength();
    uint16_t frameLength = jpglen;
    byte wCount = 0; // For counting # of writes
    while (jpglen > 0)
    {
        // read 32 bytes at a time;
        uint8_t *buffer;
        uint8_t bytesToRead = min((uint16_t)32, jpglen); // change 32 to 64 for a speedup but may not work with all setups!
        buffer = cam.readPicture(bytesToRead);
        sendBuffer(buffer, bytesToRead);
        if (++wCount >= 64)
        { // Every 2K, give a little feedback so it doesn't appear locked up
            Serial.print('.');
            wCount = 0;
        }
        jpglen -= bytesToRead;
    }
    return frameLength;
}

void VideoCapture::writeHead()
{
    String header = getHeader();
    writeAString(header);
}

void VideoCapture::writeClose()
{
    String closing = getEndHeader();
    writeAString(closing);
}

String VideoCapture::getEndHeader()
{
    return Hyphen.deviceID() + "%____%END";
}

String VideoCapture::getHeader()
{
    String filename = Hyphen.deviceID() + "-" + Time.format(TIME_FORMAT_ISO8601_FULL) + ".jpg";
    return filename;
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
int VideoCapture::setOffsetMultiple(String value)
{
    int val = Utils::parseCloudFunctionInteger(value, name());

    if (val == 0)
    {
        return 0;
    }
    if (validAddress())
    {
        config = {1, (u_int8_t)val};
        Persist.put(eepromAddress, config);
    }

    offset = val;
    return val;
}

/**
 * @private
 *
 * validAddress
 *
 * Checks to see that the eepromAddress suppled is valid
 */
bool VideoCapture::validAddress()
{
    return boots->doesNotExceedsMaxAddressSize(eepromAddress);
}

void VideoCapture::setOffsetValue()
{
    if (Utils::validConfigIdentity(config.version))
    {
        offset = (int)config.offset;
        offsetCount = 0;
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
void VideoCapture::pullStoredConfig()
{
    Utils::log("ADDRESS_PULLED FOR " + name(), "value: " + String(eepromAddress));
    if (validAddress())
    {
        Persist.get(eepromAddress, config);
        setOffsetValue();
    }
}
/**
 * @private
 *
 * requestAddress
 *
 * Gets the EEPROM Address
 *
 * @return void
 */
void VideoCapture::requestAddress()
{
    eepromAddress = boots->registerAddress(name(), sizeof(VideoCaptureStruct));
}
