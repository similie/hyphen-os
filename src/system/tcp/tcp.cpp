#include "system/tcp.h"

bool TCPClient::status()
{
    return true;
}
bool TCPClient::connect(const char *, uint16_t)
{
    return true;
}
bool TCPClient::connect(String url, uint16_t port)
{
    return true;
}
void TCPClient::stop()
{
}
// bool connected();
bool TCPClient::write(const uint8_t *buffer, size_t length, int timeoutMAYBE)
{
    return true;
}
bool TCPClient::write(const uint8_t *buffer, size_t length)
{
    return true;
}