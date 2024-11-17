#include "system/logger.h"

void LoggerClass::info(String content)
{
}
void LoggerClass::info(const char *format, ...)
{
    va_list args;
    StringFormat(format, args);
}

void LoggerClass::info(String format, ...)
{
    va_list args;
    StringFormat(format.c_str(), args);
}