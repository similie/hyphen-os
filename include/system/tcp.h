
#ifndef tcp_client_h
#define tcp_client_h
#include <iostream>
#include <Arduino.h>
#include "Hyphen.h"
class TCPClient
{
public:
    bool status();
    int connect(const char *, uint16_t);
    int connect(String, uint16_t);
    void stop();
    // bool connected();
    // size_t write(const uint8_t *, size_t, int);
    size_t write(const uint8_t *, size_t);
    // int read(uint8_t *buffer, size_t size);
};
#endif
