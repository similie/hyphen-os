#include <Ticker.h>
#ifndef timer_h
#define timer_h
class SystemTimer
{
public:
    SystemTimer(unsigned long period, void (*callback)(), bool repeat = true);
    SystemTimer(unsigned long period, void (*callback)());
    SystemTimer(const unsigned int period, void (*callback)());
    SystemTimer(void (*callback)());
    bool isActive();
    void start();
    void stop();
    void changePeriod(unsigned long period);
    void reset();

private:
    Ticker _ticker;
    float _period;
    void (*_callback)();
    bool _repeat;
};
#endif
