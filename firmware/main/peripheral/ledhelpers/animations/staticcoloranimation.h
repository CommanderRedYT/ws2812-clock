#pragma once

// local includes
#include "peripheral/ledhelpers/ledanimation.h"

namespace animation {

class StaticColorAnimation : public LedAnimation
{
    using Base = LedAnimation;

    espchrono::milliseconds32 getUpdateInterval() const override { return espchrono::milliseconds32{0}; }

    constexpr bool rendersOnce() const override { return true; }

private:

    void render_segment(SevenSegmentDigit::Segment segment, SevenSegmentDigit& sevenSegmentDigit, CRGB* leds, size_t leds_length) override;

    void render_dot(ClockDot& clockDot, CRGB* leds, size_t leds_length) override;

    LedAnimationName getEnumValue() const override { return LedAnimationName::StaticColor; }
};

} // namespace animation
