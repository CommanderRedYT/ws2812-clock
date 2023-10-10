#pragma once

// 3rdparty lib includes
#include "espchrono.h"

namespace espclock {

espchrono::time_zone get_default_timezone() noexcept;

void begin();

void update();

void syncNow();

bool isSynced();

} // namespace espclock
