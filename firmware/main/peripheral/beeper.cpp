#include "beeper.h"

constexpr const char * const TAG = "Beeper";

// system includes
#include <iterator>
#include <utility>
#include <cmath>

// esp-idf includes
#include <esp_log.h>

// 3rdparty lib includes
#include <espchrono.h>

// local includes
#include "utils/config.h"

namespace beeper {

namespace {

/*
constexpr auto beeperChannel = ledc_channel_t ::LEDC_CHANNEL_1;
constexpr auto beeperFrequency = 2700;
constexpr auto beeperResolution = ledc_timer_bit_t::LEDC_TIMER_13_BIT;
constexpr auto beeperMaxDuty = (1<<beeperResolution);
constexpr auto beeperTimer = ledc_timer_t::LEDC_TIMER_1;
*/

bool initialized{false};

} // namespace

void begin()
{
    initialized = true;
}

void update()
{
    if (!initialized)
        return;
}

} // namespace beeper
