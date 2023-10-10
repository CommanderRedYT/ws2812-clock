#pragma once

// system includes
#include <expected>
#include <optional>
#include <string>

// 3rdparty lib includes
#include "FastLED.h"
#include "arrayview.h"
#include "espchrono.h"

// local includes
#include "peripheral/ledmanager.h"

namespace animation {

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

    virtual void render_segment(SevenSegmentDigit::Segment segment, SevenSegmentDigit& sevenSegmentDigit, CRGB* leds, size_t leds_length)
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

    virtual const char * getName() const = 0;

    virtual espchrono::milliseconds32 getUpdateInterval() const = 0;

    virtual constexpr bool rendersOnce() const { return false; }

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

std::expected<void, std::string> updateAnimation(const char* name, ledmanager::LedArray& leds);

bool animationExists(const char* name);
bool animationExists(const std::string& name);

} // namespace animation
