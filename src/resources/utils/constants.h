#define DEFAULT_PUB_INTERVAL 1

#define MAX_DEVICES 7
#define EPROM_ADDRESS 0
#define MINUTE_IN_SECONDS 60
#define MILLISECOND 1000
#define NO_VALUE -9999
#define TIMEZONE_SET 8
#define ATTEMPT_THRESHOLD 3

#define MAX_SEND_TIME 15

#define SERIAL_COMMS_BAUD 76800 // 9600

#define MAX_U16 65535
#define MAX_EEPROM_ADDRESS 8197

#define DEBUG_WIRE_DISCONNECT 0
#define DEVICE_BYTE_BUFFER_SIZE 48

#ifndef constant_values_h

#define constant_values_h

class Constants
{
public:
    static const unsigned int ONE_MINUTE = 1 * MILLISECOND * MINUTE_IN_SECONDS;
    static const size_t OVERFLOW_VAL = MAX_SEND_TIME + 5;
    static const unsigned int HEARTBEAT_TIMER = MILLISECOND * MINUTE_IN_SECONDS * 15;
    static const unsigned int OFFLINE_CHECK_INTERVAL = MILLISECOND * 11;
};

#endif