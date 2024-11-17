
#ifndef tcp_client_h
#define tcp_client_h
#include <iostream>
#include <Arduino.h>
class TCPClient
{
public:
    bool status();
    bool connect(const char *, uint16_t);
    bool connect(String, uint16_t);
    void stop();
    // bool connected();
    bool write(const uint8_t *, size_t, int);
    bool write(const uint8_t *, size_t);
    // int read(uint8_t *buffer, size_t size);
};
#endif
