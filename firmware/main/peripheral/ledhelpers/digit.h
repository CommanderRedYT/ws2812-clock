#pragma once

// system includes
#include <algorithm>
#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <functional>

// 3rdparty lib includes
#include <FastLED.h>

class SevenSegmentDigit
{
public:
    explicit SevenSegmentDigit(CRGB* startLed, size_t length, size_t ledsPerSegment = 8);

    SevenSegmentDigit(CRGB* startLed, const size_t length, const CRGB& color, const size_t ledsPerSegment = 8)
        : SevenSegmentDigit{startLed, length, ledsPerSegment}
    {
        setColor(color);
    }

    void setDigit(const uint8_t digit) { setChar('0' + digit); }

    void setDigit(const uint8_t digit, const CRGB& color) { setChar('0' + digit, color); }

    bool setChar(char c)
    {
        if (m_digit != c)
        {
            m_digit = c;
            return true;
        }

        return false;
    }

    bool setChar(char c, const CRGB& color)
    {
        setColor(color);

        if (m_digit != c)
        {
            m_digit = c;
            return true;
        }

        return false;
    }

    void setColor(const CRGB& color)
    {
        std::ranges::fill(m_segmentColors, color);
        m_isStaticColor = true;
    }

    void clear() { m_digit.reset(); }

    void renderMask() const;

    std::string toString() const;

    enum Segment
    {
        F = 0,
        G,
        E,
        D,
        C,
        B,
        A,
        LAST_SEGMENT = A,
        ALL_SEGMENTS = -1,
    };

    void forEverySegment(const std::function<void(Segment, CRGB*, CRGB*, size_t)>& func) const;

    size_t length() const { return m_length; }

    CRGB* begin() const { return m_startLed; }

    CRGB* end() const { return m_startLed + m_length; }

private:

    void fillSegment(Segment segment, const CRGB& color) const;

    void clearSegment(const Segment segment) const { fillSegment(segment, CRGB::Black); }

    // void setSegmentAnimation(Segment segment, animation_t animation);

    CRGB* m_startLed;
    size_t m_length;
    size_t m_ledsPerSegment;

    bool m_isStaticColor;

    std::array<CRGB, 7> m_segmentColors;

    std::optional<char> m_digit;
};
