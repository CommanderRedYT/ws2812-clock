#pragma once

// local includes
#include "peripheral/ledhelpers/ledanimation.h"

namespace animation::internal {

class OtaAnimation : public LedAnimation
{
    using Base = LedAnimation;

    void update() override;

    espchrono::milliseconds32 getUpdateInterval() const override { return espchrono::milliseconds32{50}; }

    constexpr RenderType renderType() const override { return RenderType::ForEveryDigit; }

    cpputils::ColorHelper getPrimaryColor() const override { return cpputils::ColorHelper{0, 255, 0}; }

    void render_digit(SevenSegmentDigit& sevenSegmentDigit, size_t digit_idx, CRGB* leds, size_t length) override;

    void render_dot(ClockDot& clockDot, CRGB* leds, size_t leds_length) override;

    constexpr bool shouldSetDigits() const override { return false; }

private:
    uint8_t m_percentage[3]{0, 0, 0};
    uint8_t m_digit_count{0};
};

extern LedAnimation* otaAnimation;

} // namespace animation::internal
