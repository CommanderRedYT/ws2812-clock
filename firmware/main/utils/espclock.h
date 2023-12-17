#pragma once

// system includes
#include <optional>

// 3rdparty lib includes
#include <espchrono.h>

namespace espclock {

espchrono::time_zone get_default_timezone() noexcept;

void begin();

void update();

void syncNow();

bool isSynced();

const std::optional<espchrono::utc_clock::time_point>& sunrise();

const std::optional<espchrono::utc_clock::time_point>& sunset();

bool isNight();

} // namespace espclock
