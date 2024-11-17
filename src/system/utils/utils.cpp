
#include "system/utils.h"
String StringFormat(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    size_t size = vsnprintf(nullptr, 0, format, args) + 1; // Extra space for '\0'
    va_end(args);

    char *buffer = new char[size];
    if (!buffer)
    {
        return String("");
    }

    va_start(args, format);
    vsnprintf(buffer, size, format, args);
    va_end(args);

    String result(buffer);
    delete[] buffer;
    return result;
}

bool waitFor(std::function<bool()> condition, unsigned long timeout)
{
    unsigned long start = millis();
    while (!condition())
    {
        if (millis() - start >= timeout)
        {
            return false; // Timeout occurred
        }
        // Perform background tasks to keep the system responsive
        Hyphen.process();
        vTaskDelay(1); // Yield to other tasks (optional)
    }
    return true; // Condition met within timeout
}