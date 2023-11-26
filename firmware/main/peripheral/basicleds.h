#pragma once

// system includes
#include <cstdint>
#include <optional>

// 3rdparty lib includes
#include <espchrono.h>

namespace basicleds {

struct Led {
    uint16_t current{};
    uint16_t target{};
    std::optional<espchrono::millis_clock::time_point> lastUpdate;
};

struct LedConfig
{
    Led ap_status_led;
    Led wifi_status_led;
    Led alarm_status_led;
};

extern LedConfig ledConfig;

void begin();

void update();

} // namespace basicleds
