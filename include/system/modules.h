#ifndef MODULES_H
#define MODULES_H
#include "system/persistence.h"
#include "system/fuel.h"
#include "system/tcp.h"
#include "system/time.h"
#include "system/utils.h"

// Declare the global instance

extern FuelGaugeClass FuelGauge;

extern TimeClass Time;

extern Persistence Persist;

#endif