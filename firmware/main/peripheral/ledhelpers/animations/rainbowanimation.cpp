#include "rainbowanimation.h"

namespace animation {

void RainbowAnimation::update()
{
    Base::update();

    m_hue += 1;
}

void RainbowAnimation::render_all(CRGB* leds, size_t leds_length)
{
    Base::render_all(leds, leds_length);

    fill_rainbow(leds, leds_length, m_hue, 1);
}


} // namespace animation
