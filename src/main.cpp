
/*
 * Project hyphen-community
 * Description: A never fail basic runtime for our particle ESP32 devices
 * Author: Similie - Adam Smithee
 * License: MIT
 * Date: Started 1 of November, 2024.
 */
#include <Arduino.h>
#include "resources/devices/device-manager.h"
#define DEBUGGER true // set to false for field devices
LocalProcessor processor;
// MqttProcessor processor(&boots);
DeviceManager manager(&processor, DEBUGGER);
// BUILD_TIMESTAMP is a macro defined at compile time:
#ifndef BUILD_TIMESTAMP
#error "BUILD_TIMESTAMP not defined!"
#endif

void initClockFromBuildTime()
{
  // Build timestamp is in seconds since 1970
  const time_t t = BUILD_TIMESTAMP;
  struct timeval tv = {.tv_sec = t, .tv_usec = 0};
  if (settimeofday(&tv, nullptr) != 0)
  {
    // handle error...
  }
}

void setup()
{
  Serial.begin(115200);
  initClockFromBuildTime();
  // Install the GPIO ISR service
  gpio_install_isr_service(ESP_INTR_FLAG_LEVEL1 | ESP_INTR_FLAG_EDGE);

  FuelGauge.init();
  manager.init();
  disableCore0WDT();
}
// loop() runs over and over again, as quickly as it can execute
void loop()
{
  manager.loop();
}
