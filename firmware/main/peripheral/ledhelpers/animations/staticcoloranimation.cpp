#include "staticcoloranimation.h"

// local includes
#include "utils/config.h"

namespace animation {

void StaticColorAnimation::render_segment(SevenSegmentDigit::Segment segment, SevenSegmentDigit& sevenSegmentDigit, CRGB* leds, size_t leds_length)
{
    Base::render_segment(segment, sevenSegmentDigit, leds, leds_length);

    auto& primaryColor = configs.primaryColor.value();
    auto* startLed = sevenSegmentDigit.begin();
    auto* endLed = sevenSegmentDigit.end();
    fill_solid(startLed, std::distance(startLed, endLed), CRGB(primaryColor.r, primaryColor.g, primaryColor.b));
}

void StaticColorAnimation::render_dot(ClockDot& clockDot, CRGB* leds, size_t leds_length)
{
    Base::render_dot(clockDot, leds, leds_length);

    auto& secondaryColor = configs.secondaryColor.value();
    auto& tertiaryColor = configs.tertiaryColor.value();
    auto* startLed = clockDot.begin();
    size_t length = clockDot.length();
    bool on = clockDot.on();

    fill_solid(
           startLed,
           length,
           on ?
            CRGB(secondaryColor.r, secondaryColor.g, secondaryColor.b) :
            CRGB(tertiaryColor.r, tertiaryColor.g, tertiaryColor.b)
    );
}

} // namespace animation
