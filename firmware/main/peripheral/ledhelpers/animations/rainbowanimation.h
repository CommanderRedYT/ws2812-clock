#pragma once

// local includes
#include "peripheral/ledhelpers/ledanimation.h"

namespace animation {

class RainbowAnimation : public LedAnimation
{
    using Base = LedAnimation;

    espchrono::milliseconds32 getUpdateInterval() const override { return espchrono::milliseconds32{5}; }

    constexpr bool rendersOnce() const override { return true; }

private:

    void update() override;

    void render_all(CRGB* leds, size_t leds_length) override;

    LedAnimationName getEnumValue() const override { return LedAnimationName::Rainbow; }

    uint8_t m_hue{0};
};

} // namespace animation
