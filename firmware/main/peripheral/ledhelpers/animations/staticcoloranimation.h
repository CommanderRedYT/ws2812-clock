#pragma once

// local includes
#include "peripheral/ledhelpers/ledanimation.h"

namespace animation {

class StaticColorAnimation : public LedAnimation
{
    using Base = LedAnimation;

    espchrono::milliseconds32 getUpdateInterval() const override { return espchrono::milliseconds32{0}; }

    constexpr RenderType renderType() const override { return RenderType::AllAtOnce; }

    cpputils::ColorHelper getPrimaryColor() const override { return cpputils::ColorHelper{0, 0, 0}; }

    void render_all(CRGB* leds, size_t length) override;

    void render_dot(ClockDot& clockDot, CRGB* leds, size_t leds_length) override;

    std::optional<LedAnimationName> getEnumValue() const override { return LedAnimationName::StaticColor; }
};

} // namespace animation
