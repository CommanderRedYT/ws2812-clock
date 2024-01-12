#pragma once

// local includes
#include "peripheral/ledhelpers/ledanimation.h"

namespace animation {

class NewYearAnimation : public LedAnimation
{
    using Base = LedAnimation;

    espchrono::milliseconds32 getUpdateInterval() const override { return espchrono::milliseconds32{16}; }

    constexpr RenderType renderType() const override { return RenderType::AllAtOnce; }

    constexpr bool shouldSetDigits() const override { return false; }

    void update() override;

    void render_all(CRGB* leds, size_t leds_length) override;

    std::optional<LedAnimationName> getEnumValue() const override { return LedAnimationName::NewYearAnimation; }

private:
    uint8_t m_hue{0};
};

} // namespace animation
