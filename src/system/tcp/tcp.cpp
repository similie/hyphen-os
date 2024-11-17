#include "system/tcp.h"

bool TCPClient::status()
{
    return Hyphen.hyConnect().getClient().connected();
}
int TCPClient::connect(const char *host, uint16_t port)
{
    return Hyphen.hyConnect().getClient().connect(host, port);
}
int TCPClient::connect(String url, uint16_t port)
{
    return connect(url.c_str(), port);
}
void TCPClient::stop()
{
    Hyphen.hyConnect().getClient().stop();
}
size_t TCPClient::write(const uint8_t *buffer, size_t length)
{
    return Hyphen.hyConnect().getClient().write(buffer, length);
}