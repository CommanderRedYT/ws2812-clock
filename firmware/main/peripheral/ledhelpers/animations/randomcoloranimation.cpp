#include "randomcoloranimation.h"

// local includes
#include "strutils.h"
#include "utils/config.h"

namespace animation {

void RandomColorAnimation::update()
{
    Base::update();

    const auto local_now = espchrono::local_clock::now();
    const auto local_dt = toDateTime(local_now);

    const auto year = static_cast<int>(local_dt.date.year());
    const auto month = static_cast<unsigned>(local_dt.date.month());
    const auto day = static_cast<unsigned>(local_dt.date.day());
    const int magic_number = year + month + day;

    CRGB color;

    color.r = magic_number ^ 0x0000FF;
    color.g = magic_number ^ 0x00FF00;
    color.b = magic_number ^ 0xFF0000;

    m_color = color;
}


void RandomColorAnimation::render_all(CRGB* leds, const size_t leds_length)
{
    Base::render_all(leds, leds_length);

    std::fill_n(leds, leds_length, m_color);
}

} // namespace animation
