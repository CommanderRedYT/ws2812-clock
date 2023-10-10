#pragma once

namespace basicleds {

struct LedConfig
{
    bool ap_status_led;
    bool wifi_status_led;
    bool alarm_status_led;
};

extern LedConfig ledConfig;

void begin();

void update();

} // namespace basicleds
