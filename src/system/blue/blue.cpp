#include "system/blue.h"
#include "Hyphen.h"
// Initialize the static instance pointer.
BluetoothManager *BluetoothManager::_instance = nullptr;

void MyServerCallbacks::onConnect(BLEServer *pServer)
{
    Serial.println("BLE client connected");
}

void MyServerCallbacks::onDisconnect(BLEServer *pServer)
{
    Serial.println("BLE client disconnected");
    // Restart advertising when a client disconnects.
    pServer->getAdvertising()->start();
}

MyCharacteristicCallbacks::MyCharacteristicCallbacks(HyphenClass *hyphen)
{
    Hyphen = hyphen;
}

void MyCharacteristicCallbacks::manageFunction(String name, String payload)
{
    Serial.println("PRE HYPHEN");
    if (!Hyphen)
    {
        return;
    }
    String result = Hyphen->hyConnect().getSubscriptionManager().runFunction(name.c_str(), payload.c_str());
    Serial.print("MY RESULT FROM FUNCTION CALL");

    Serial.println(result);
}

void MyCharacteristicCallbacks::processCmd()
{
    // Validate basic format: should be at least "X<>" (3 chars)
    if (cmdBuffer.length() < 3)
    {
        Serial.println("Invalid command: too short.");
        cmdBuffer = "";
        return;
    }

    // The first character is the command type.
    char cmdType = cmdBuffer.charAt(0);
    // The second character must be '<'
    if (cmdBuffer.charAt(1) != '<')
    {
        Serial.println("Invalid command format: missing '<'.");
        cmdBuffer = "";
        return;
    }

    // Extract the inner payload (excluding the first two characters and the last '>')
    String inner = cmdBuffer.substring(2, cmdBuffer.length() - 1);

    // Optionally, split the inner part into a name and payload if a colon ':' exists.
    String name = "";
    String payload = inner;
    int colonIndex = inner.indexOf(':');
    if (colonIndex != -1)
    {
        name = inner.substring(0, colonIndex);
        payload = inner.substring(colonIndex + 1);
    }

    // Log the parsed command
    Serial.print("Command Type: ");
    Serial.println(cmdType);
    Serial.print("Name: ");
    Serial.println(name);
    Serial.print("Payload: ");
    Serial.println(payload);

    // Process the command based on its type.
    switch (static_cast<LoggingDetails>(cmdType))
    {
    case FUNCTION:
        Serial.println("Processing FUNCTION command");
        // Process function command using name and payload.
        manageFunction(name, payload);
        break;
    case CONFIG:
        Serial.println("Processing CONFIG command");
        // Process configuration command.
        break;
    case VARIABLE:
        Serial.println("Processing VARIABLE command");
        // Process variable command.
        break;
    case LOGGING:
        Serial.println("Processing LOGGING command");
        // Process logging command.
        break;
    // Add additional cases as needed.
    default:
        Serial.println("Unknown command type received.");
        break;
    }

    // Clear the buffer for the next command.
    cmdBuffer = "";
}

void MyCharacteristicCallbacks::onWrite(BLECharacteristic *pCharacteristic)
{
    std::string value = pCharacteristic->getValue();
    if (value.length() <= 0)
    {
        return;
    }
    Serial.print("Received config data: ");
    Serial.println(value.c_str());
    // Append the received chunk to the buffer.
    cmdBuffer += String(value.c_str());
    if (!cmdBuffer.endsWith(">"))
    {
        return;
    }
    processCmd();
    // Process configuration data here.
}

BluetoothManager::BluetoothManager()
    : pServer(nullptr), pService(nullptr), pCharacteristic(nullptr), bleEnabled(false),
      _switchTriggered(false), _switchPressed(false), _pressStart(0)
{
    _instance = this;
    pinMode(SWITCH_PIN, INPUT);
}

void BluetoothManager::init(HyphenClass *hyphen)
{
    Hyphen = hyphen;
    if (started)
    {
        return;
    }
    //  started = true;

    // attachInterrupt(digitalPinToInterrupt(SWITCH_PIN), BluetoothManager::staticSwitchISR, CHANGE);
    Serial.println("BluetoothManager initialized. Switch interrupt attached.");
}

String BluetoothManager::generateServiceUUID()
{
    uint64_t mac = ESP.getEfuseMac(); // 48-bit MAC address, returned as a 64-bit integer.
    // Convert the MAC to a hex string.
    char macStr[13];
    sprintf(macStr, "%012llX", mac);
    // Insert the MAC into a base UUID.
    // For example, using the base "0000XXXX-0000-1000-8000-00805F9B34FB"
    // where XXXX is replaced by the last 4 characters of the MAC.
    String uuid = "0000";
    uuid += String(macStr).substring(8, 12); // Use the last 4 hex digits.
    uuid += "-0000-1000-8000-00805F9B34FB";
    return uuid;
}

void BluetoothManager::enableBLE()
{

    if (bleEnabled || !Hyphen || !started)
    {
        return;
    }
    Serial.println("WORKIGN IT");
    String name = "Hy_" + Hyphen->deviceID();
    Serial.print("MY BL NAME ");
    Serial.println(name);
    // Initialize BLE with device name (used as a default).
    BLEDevice::init(name.c_str());

    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());
    String service_uuid = generateServiceUUID();
    // Create service and characteristic.
    Serial.printf("BOOMO AS %s \n", service_uuid.c_str());
    pService = pServer->createService(service_uuid.c_str());
    pCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_UUID,
        BLECharacteristic::PROPERTY_READ |
            BLECharacteristic::PROPERTY_WRITE | // Added write property.
            BLECharacteristic::PROPERTY_NOTIFY);
    pCharacteristic->addDescriptor(new BLE2902());
    pCharacteristic->setCallbacks(new MyCharacteristicCallbacks(Hyphen));
    pService->start();

    // Set up advertisement data.
    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();

    // Build advertisement data with complete local name.
    BLEAdvertisementData advertisementData;
    advertisementData.setName(name.c_str());
    advertisementData.setFlags(ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT);
    pAdvertising->setAdvertisementData(advertisementData);

    // Optionally, also set scan response data.
    BLEAdvertisementData scanResponseData;
    scanResponseData.setName(name.c_str());
    pAdvertising->setScanResponseData(scanResponseData);

    // Add your service UUID so that the mobile app can filter by service.
    pAdvertising->addServiceUUID(service_uuid.c_str());

    // Start advertising.
    pAdvertising->start();

    bleEnabled = true;
    Serial.println("BLE enabled and advertising started.");
}

void BluetoothManager::disableBLE()
{
    if (!bleEnabled)
        return;
    if (pServer)
    {
        pServer->disconnect(pServer->getConnId());
    }
    bleEnabled = false;
    Serial.println("BLE disabled.");
}

bool BluetoothManager::isEnabled()
{
    return bleEnabled;
}

void BluetoothManager::log(const String &message, LoggingDetails prefix)
{
    if (!pCharacteristic || !bleEnabled)
    {
        return;
    }

    int mtu = BLEDevice::getMTU() - 6; // Subtract 3 bytes for the ATT header and prefix
    int len = message.length();
    int offset = 0;

    while (offset < len)
    {
        String chunk = message.substring(offset, offset + mtu);
        String send = String(prefix) + '<' + chunk;
        offset += mtu;
        if (offset >= len - 1)
        {
            send += '>';
        }
        pCharacteristic->setValue(send.c_str());
        pCharacteristic->notify();

        delay(10); // Small delay between notifications
    }
    // Serial.printf("Logged message: %s\n", message.c_str());

    // pCharacteristic->setValue(message.c_str());
    // pCharacteristic->notify();
    // Serial.printf("Logged message: %s\n", message.c_str());
}

// New member function to update the BLE device name.
void BluetoothManager::setDeviceName(const char *newName)
{
    // Use the ESP-IDF API to update the device name.
    esp_err_t err = esp_ble_gap_set_device_name(newName);
    if (err == ESP_OK)
    {
        Serial.printf("Device name updated to: %s\n", newName);
    }
    else
    {
        Serial.printf("Failed to update device name. Error: %d\n", err);
    }
}

void BluetoothManager::loop()
{
    if (!_switchTriggered)
    {
        return;
    }
    _switchTriggered = false;
    unsigned long pressDuration = millis() - _pressStart;
    Serial.print("Switch press duration: ");
    Serial.print(pressDuration);
    Serial.println(" ms");

    if (pressDuration < SHORT_PRESS_THRESHOLD && !bleEnabled)
    {
        Serial.println("Short press detected: enabling BLE.");
        enableBLE();
    }
    else if (pressDuration >= LONG_PRESS_THRESHOLD && bleEnabled)
    {
        Serial.println("Long press detected: disabling BLE.");
        disableBLE();
    }
    // Additional BLE tasks can be processed here.
}

void BluetoothManager::switchISR()
{
    int state = digitalRead(SWITCH_PIN);
    if (state == LOW)
    {
        _pressStart = millis();
        _switchPressed = true;
    }
    else if (_switchPressed)
    {
        _switchTriggered = true;
        _switchPressed = false;
    }
}

void IRAM_ATTR BluetoothManager::staticSwitchISR()
{
    if (_instance)
    {
        _instance->switchISR();
    }
}