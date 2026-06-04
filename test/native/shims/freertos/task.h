// freertos/task.h — native shim. vTaskDelay is a no-op so timing in tests is
// driven solely by the fake clock.
#pragma once

#include <freertos/FreeRTOS.h>

inline void vTaskDelay(TickType_t) {}
inline TaskHandle_t xTaskGetCurrentTaskHandle() { return nullptr; }
inline void vTaskDelete(TaskHandle_t) {}
inline BaseType_t xTaskCreatePinnedToCore(void (*)(void*), const char*, uint32_t,
                                          void*, UBaseType_t, TaskHandle_t*,
                                          BaseType_t) {
  return pdTRUE;
}
