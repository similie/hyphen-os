#ifndef BLUETOOTHMANAGER_H
#define BLUETOOTHMANAGER_H
#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include "esp_gap_ble_api.h"
// #include <ESP32Lib.h> // Or use ESP.getEfuseMac() if available

class HyphenClass;
// UUIDs for your BLE service and characteristic.
// #define SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

// Pin and timing thresholds for the momentary switch.
#define SWITCH_PIN 34
#define SHORT_PRESS_THRESHOLD 1000 // ms: short press (enable BLE)
#define LONG_PRESS_THRESHOLD 2000  // ms: long press (disable BLE)
// class HyphenClass;
enum LoggingDetails
{
    LOGGING = 'L',
    CONFIG = 'C',
    CONFIG_CONFIRMATION = 'Z',
    PAYLOAD = 'P',
    LOGGING_PREFIX_WARNING = 'W',
    LOGGING_PREFIX_ERROR = 'E',
    FUNCTION = 'F',
    VARIABLE = 'V',
};

class MyCharacteristicCallbacks : public BLECharacteristicCallbacks
{
private:
    String cmdBuffer = "";
    void processCmd();
    void manageFunction(String, String);
    HyphenClass *Hyphen;

public:
    MyCharacteristicCallbacks(HyphenClass *);
    void onWrite(BLECharacteristic *pCharacteristic) override;
};

class MyServerCallbacks : public BLEServerCallbacks
{
public:
    void onConnect(BLEServer *pServer) override;
    void onDisconnect(BLEServer *pServer) override;
};

class BluetoothManager
{
public:
    BluetoothManager();
    String generateServiceUUID();
    // Initializes the Bluetooth manager and attaches the switch interrupt.
    void init(HyphenClass *);
    // Call from your main loop to process switch events and BLE tasks.
    void loop();

    // Logging function: sends string messages to connected BLE clients.
    void log(const String &message, LoggingDetails prefix);

    // Methods to explicitly enable/disable BLE.
    void enableBLE();
    void disableBLE();
    bool isEnabled();

    // New member function to update the device name at runtime.
    void setDeviceName(const char *newName);

    // ISR handling for the momentary switch.
    void switchISR();
    static void IRAM_ATTR staticSwitchISR();

private:
    BLEServer *pServer;
    BLEService *pService;
    BLECharacteristic *pCharacteristic;
    bool bleEnabled;
    bool started = false;
    HyphenClass *Hyphen;
    // Variables for momentary switch handling.
    volatile bool _switchTriggered;
    volatile bool _switchPressed;
    volatile unsigned long _pressStart;

    // Static pointer for ISR access.
    static BluetoothManager *_instance;
};

#endif // BLUETOOTHMANAGER_H
