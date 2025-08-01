#ifndef MODULES_H
#define MODULES_H
#include "system/blue.h"
#include "system/persistence.h"
#include "system/fuel.h"
#include "system/tcp.h"
#include "system/time.h"
#include "system/sd-card.h"
#include "system/utils.h"
#include "system/watchdog.h"
// Declare the global instance

extern WatchdogClass Watchdog;
extern FuelGaugeClass FuelGauge;
extern TimeClass Time;
extern Persistence Persist;
extern SDCard Storage;
extern BluetoothManager Blue;
#endif