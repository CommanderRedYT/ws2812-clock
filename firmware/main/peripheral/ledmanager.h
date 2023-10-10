#pragma once

// system includes
#include <array>
#include <string>
#include <utility>

// 3rdparty lib includes
#include <delayedconstruction.h>
#include <espchrono.h>
#include <wrappers/recursive_mutex_semaphore.h>

// local includes
#include "ledhelpers/digit.h"
#include "ledhelpers/clockdot.h"

namespace ledmanager {

using LedArray = std::array<CRGB, HARDWARE_WS2812B_COUNT>;

class LedManager
{
    using Digits = std::array<SevenSegmentDigit, 4>;

public:
    LedManager(Digits digits, ClockDot upper, ClockDot lower)
        : digits{std::move(digits)}, upper{upper}, lower{lower}
    {}

    Digits digits;
    ClockDot upper;
    ClockDot lower;

    void render();

    void setVisible(bool visible) { m_visible = visible; }

    void handleVoltageAndCurrent();

    static uint16_t getFps();

private:

    std::string toString() const;

    bool m_visible{};

    uint8_t m_brightness{0};
    espchrono::millis_clock::time_point m_brightnessLastUpdate{};
};

extern cpputils::DelayedConstruction<LedManager> ledManager;

extern cpputils::DelayedConstruction<espcpputils::recursive_mutex_semaphore> led_lock;

void begin();

void update();

const LedArray& getLeds();

} // namespace ledmanager
