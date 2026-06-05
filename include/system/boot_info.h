#pragma once
#include <stdint.h>

// Field visibility into resets. Captures the cause of the most recent reset
// (esp_reset_reason) and a persisted boot counter so the cloud/console can SEE
// whether the external hardware watchdog — or a panic/brownout — is actually
// firing in the field. Previously there was no way to tell "the dog saved it"
// from "it never hung."
//
// Caveat: an external watchdog that cuts the power rail looks like ESP_RST_POWERON
// (same as a normal power-on / user power-cycle). The signal is the *trend* — a
// device that keeps coming up POWERON with a climbing boot count while it should
// be running is being reset by something. A clean Hyphen::reset() shows ESP_RST_SW.
namespace hyphen {
namespace boot {

// Call ONCE early in setup() (after nvs_flash_init — i.e. not from a global
// constructor, which would hit the same dead-NVS bug we fixed for time).
void capture();

const char *resetReasonStr(); // short tag: POWERON / SW / PANIC / BROWNOUT / TASK_WDT / ...
int resetReasonCode();        // raw esp_reset_reason_t value
unsigned long bootCount();    // persisted; increments once per boot

}  // namespace boot
}  // namespace hyphen
