#include <Arduino.h>
String StringFormat(const char *format, ...);
bool waitFor(std::function<bool()> condition, unsigned long timeout);