#include "system/boot_info.h"
#include <esp_system.h>
#include <Preferences.h>

namespace hyphen {
namespace boot {

static int s_reason = 0; // ESP_RST_UNKNOWN
static unsigned long s_boots = 0;
static bool s_done = false;

const char *resetReasonStr()
{
    switch ((esp_reset_reason_t)s_reason)
    {
    case ESP_RST_POWERON:   return "POWERON";   // power applied (incl. external WD power-cut)
    case ESP_RST_EXT:       return "EXT";       // reset pin asserted
    case ESP_RST_SW:        return "SW";        // esp_restart() / Hyphen::reset()
    case ESP_RST_PANIC:     return "PANIC";     // exception / abort
    case ESP_RST_INT_WDT:   return "INT_WDT";   // interrupt watchdog
    case ESP_RST_TASK_WDT:  return "TASK_WDT";  // task watchdog
    case ESP_RST_WDT:       return "WDT";       // other watchdog
    case ESP_RST_DEEPSLEEP: return "DEEPSLEEP";
    case ESP_RST_BROWNOUT:  return "BROWNOUT";  // brownout detector (or a shallow power-cut)
    case ESP_RST_SDIO:      return "SDIO";
    default:                return "UNKNOWN";
    }
}

int resetReasonCode() { return s_reason; }
unsigned long bootCount() { return s_boots; }

void capture()
{
    if (s_done)
    {
        return;
    }
    s_done = true;
    s_reason = (int)esp_reset_reason();
    // Own NVS namespace; opened here (setup time) where NVS is ready — never in a
    // global constructor. One small write per boot is well within NVS endurance.
    Preferences p;
    if (p.begin("hyphen_boot", false))
    {
        s_boots = p.getULong("boots", 0) + 1;
        p.putULong("boots", s_boots);
        p.end();
    }
}

}  // namespace boot
}  // namespace hyphen
