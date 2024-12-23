#pragma once

// system includes
#include <expected>
#include <optional>
#include <string>

// 3rdparty lib includes
#include <FastLED.h>
#include <arrayview.h>
#include <color_utils.h>
#include <espchrono.h>

// local includes
#include "peripheral/ledmanager.h"

#define LedAnimationNameValues(x) \
    x(Rainbow) \
    x(StaticColor) \
    x(NewYearAnimation)           \
    x(Strobo)
DECLARE_GLOBAL_TYPESAFE_ENUM(LedAnimationName, : uint8_t, LedAnimationNameValues);

namespace animation {

enum RenderType {
    AllAtOnce,
    ForEveryDigit,
    ForEverySegment,
};

class LedAnimation
{
public:

    void start(CRGB* leds, size_t leds_length)
    {
        init(leds, leds_length);

        m_lastRender = espchrono::millis_clock::now();
    }

    void stop(CRGB* leds, size_t leds_length)
    {
        deinit(leds, leds_length);

        m_lastRender = espchrono::millis_clock::now();
    }

    virtual void update()
    {
        m_lastUpdate = espchrono::millis_clock::now();
    }

    virtual void render_segment(SevenSegmentDigit::Segment segment, SevenSegmentDigit& sevenSegmentDigit, CRGB* start_led, CRGB* end_led, size_t length)
    {
        m_lastRender = espchrono::millis_clock::now();
    }

    virtual void render_digit(SevenSegmentDigit& sevenSegmentDigit, size_t digit_idx, CRGB* leds, size_t leds_length)
    {
        m_lastRender = espchrono::millis_clock::now();
    }

    virtual void render_all(CRGB* leds, size_t leds_length)
    {
        m_lastRender = espchrono::millis_clock::now();
    }

    virtual void render_dot(ClockDot& clockDot, CRGB* leds, size_t leds_length)
    {
        m_lastRender = espchrono::millis_clock::now();
    }

    virtual std::optional<LedAnimationName> getEnumValue() const { return std::nullopt; };

    virtual espchrono::milliseconds32 getUpdateInterval() const = 0;

    virtual cpputils::ColorHelper getPrimaryColor() const { return cpputils::ColorHelper{0, 0, 0}; }

    virtual constexpr RenderType renderType() const { return RenderType::AllAtOnce; }

    virtual constexpr bool shouldSetDigits() const { return true; }

    bool needsUpdate() const;

    /*
    void setSpeedMultiplier(float speedMultiplier)
    {
        m_speedMultiplier = speedMultiplier;
    }

    float getSpeedMultiplier() const
    {
        return m_speedMultiplier;
    }
    */

private:

    virtual void init(CRGB* leds, size_t leds_length) {}

    virtual void deinit(CRGB* leds, size_t leds_length) {}

    // float m_speedMultiplier{1};

protected:
    std::optional<espchrono::millis_clock::time_point> m_lastRender;
    std::optional<espchrono::millis_clock::time_point> m_lastUpdate;
};

extern cpputils::ArrayView<LedAnimation*> animations;

extern LedAnimation* currentAnimation;

const LedAnimation& getFirstAnimation();

std::expected<void, std::string> updateAnimation(LedAnimationName enumValue, ledmanager::LedArray& leds);

bool animationExists(LedAnimationName enumValue);

} // namespace animation
