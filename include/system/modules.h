#ifndef MODULES_H
#define MODULES_H
#include "system/persistence.h"
// #include "connectivity/connectivity.h"
#include "system/fuel.h"
// #include "system/logger.h"
#include "system/sleep.h"
#include "system/tcp.h"
#include "system/time.h"
#include "system/utils.h"

// Declare the global instance

extern FuelGaugeClass FuelGauge;

extern SystemSleepConfigurationClass SystemSleepConfiguration;

extern TimeClass Time;

extern Persistence Persist;

// extern LoggerClass Log;

#endif