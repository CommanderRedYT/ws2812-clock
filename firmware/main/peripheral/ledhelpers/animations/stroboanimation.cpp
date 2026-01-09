#include "stroboanimation.h"

// local includes
#include "utils/config.h"

namespace animation {

void StroboAnimation::update()
{
    Base::update();

    m_on = !m_on;
}

void StroboAnimation::render_all(CRGB* leds, const size_t length)
{
    Base::render_all(leds, length);

    const auto& primaryColor = configs.primaryColor.value();
    const auto color = CRGB(primaryColor.r, primaryColor.g, primaryColor.b);

    std::fill_n(leds, length, m_on ? color : CRGB::Black);
}

void StroboAnimation::render_dot(ClockDot& clockDot, CRGB* leds, const size_t leds_length)
{
    Base::render_dot(clockDot, leds, leds_length);

    const auto& secondaryColor = configs.secondaryColor.value();
    const auto& tertiaryColor = configs.tertiaryColor.value();

    auto* startLed = clockDot.begin();
    const size_t length = clockDot.length();

    const bool on = clockDot.on();

    std::fill_n(startLed, length, on ? CRGB(secondaryColor.r, secondaryColor.g, secondaryColor.b) : CRGB(tertiaryColor.r, tertiaryColor.g, tertiaryColor.b));
}

} // namespace animation
