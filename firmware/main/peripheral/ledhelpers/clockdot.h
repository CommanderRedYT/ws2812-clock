#pragma once

// system includes
#include <cstdint>
#include <optional>
#include <string>

// 3rdparty lib includes
#include <FastLED.h>

class ClockDot
{
public:
    enum DotPlacement
    {
        Top = 0,
        Bottom,
    };

    explicit ClockDot(DotPlacement placement, CRGB* startLed, size_t length);

    void render();

    void on(bool on) { m_on = on; }

    bool on() const { return m_on; }

    CRGB* begin() const { return m_startLed; }
    CRGB* end() const { return m_startLed + m_length; }
    size_t length() const { return m_length; }

    std::string toString() const;

    DotPlacement placement() const { return m_placement; }

private:
    CRGB* m_startLed;
    size_t m_length;

    bool m_on;

    DotPlacement m_placement;
};
