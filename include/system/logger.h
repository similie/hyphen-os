#ifndef logger_h
#define logger_h
#include "Arduino.h"
#include "utils.h"
class LoggerClass
{
public:
    void info(String);
    void info(const char *format, ...);
    void info(String, ...);
};
#endif