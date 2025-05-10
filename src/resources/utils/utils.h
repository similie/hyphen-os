

#ifndef Utils_h
#define Utils_h

#include "Hyphen.h"
#include "string.h"
#include "math.h"

#include "resources/utils/constants.h"
#define LOG_FILE_NAME "hyphen-logs.txt"
class Utils
{
private:
    static const bool LOG_TO_FILE = true;
    static const uint32_t MAX_LOG_SIZE = 1000000;
    String removeSensorIdFromPayload(String);

    bool notStopCheckChar(char);
    const static int FAILED_VALUE = NO_VALUE;
    void fillParseSplitReadSerial(String ourReading, size_t paramLength, size_t max, String nameMap[], float value_hold[][Constants::OVERFLOW_VAL]);

public:
    ~Utils();
    Utils();
    static void reboot();
    void splitStringToValues(String, String *, size_t);
    static double parseCloudFunctionDouble(String value, String name);
    static int parseCloudFunctionInteger(String value, String name);
    static void setDebug(bool debug);
    bool hasSerialIdentity(int identity);
    String getConvertedAddressCmd(String cmd, int identity);
    bool inValidMessageString(String message, int identity);
    static bool validConfigIdentity(uint8_t identity);
    String requestDeviceId(int identity, String cmd);
    bool serialMessageHasError(String message, int identity);
    String receiveDeviceId(int identity);
    void parseSerial(String ourReading, size_t paramLength, size_t max, float value_hold[][Constants::OVERFLOW_VAL]);
    void parseSplitReadSerial(String, size_t, size_t, String[], float value_hold[][Constants::OVERFLOW_VAL]);
    bool invalidNumber(String value);
    bool containsChar(char c, String readFrom);
    const char *stringConvert(String value);
    void insertValue(int value, int arr[], size_t size);
    void insertValue(long value, long arr[], size_t size);
    void insertValue(float value, float arr[], size_t size);
    void shift(int value, size_t index, int arr[], size_t size);
    void shift(long value, size_t index, long arr[], size_t size);
    void shift(float value, size_t index, float arr[], size_t size);
    int getMedian(int readparam, int arr[]);
    float getMax(float values[], size_t MAX);
    float getSum(float values[], size_t MAX);
    float getMedian(float arr[], size_t max);
    long getMedian(long readparam, long arr[]);
    void fillStringifiedFailedDefaults(String *, size_t);
    static bool connected();
    static void machineNameDirect(String name, byte *save);
    static String machineToReadableName(byte *restore);
    static unsigned long machineName(String name, bool unique);
    static int containsValue(String arr[], size_t size, String value);
    static int getIndexOf(String, String arr[], size_t);
    static bool containsString(String, String);
    // template<typename T>
    static String removeNewLine(String value);
    static void log(String event, String message);
    static void logToFile(String log);

    static int log(String event, String message, int errorCode);
    size_t skipMultiple(unsigned int size, size_t maxVal, unsigned int threshold);
    static int simCallback(int type, const char *buf, int len, char *value);
};

#endif