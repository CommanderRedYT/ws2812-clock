#pragma once

// local includes
#include "peripheral/ledhelpers/ledanimation.h"

namespace animation {

class RandomColorAnimation : public LedAnimation
{
    using Base = LedAnimation;

    espchrono::milliseconds32 getUpdateInterval() const override { return espchrono::milliseconds32{16}; }

    constexpr RenderType renderType() const override { return RenderType::AllAtOnce; }

    void update() override;

    void render_all(CRGB* leds, size_t leds_length) override;

    std::optional<LedAnimationName> getEnumValue() const override { return LedAnimationName::RandomColor; }

private:
    CRGB m_color;
};

} // namespace animation
