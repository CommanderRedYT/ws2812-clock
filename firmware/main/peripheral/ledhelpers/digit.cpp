#include "digit.h"

// system includes
#include <algorithm>

// 3rdparty lib includes
#include <fmt/core.h>

// local includes
#include "digithelper.h"

SevenSegmentDigit::SevenSegmentDigit(CRGB* startLed, size_t length, size_t ledsPerSegment) :
    m_startLed{startLed},
    m_length{length},
    m_ledsPerSegment{ledsPerSegment},
    m_isStaticColor{true},
    m_segmentColors{
        CRGB::White,
        CRGB::White,
        CRGB::White,
        CRGB::White,
        CRGB::White,
        CRGB::White,
        CRGB::White,
    }
{}

void SevenSegmentDigit::renderMask()
{
    uint8_t mask = digithelper::getSegmentMask(m_digit.value_or(' '));

    if (mask & 0b00000001) /*fillSegment(A, m_segmentColors[A]);*/{} else clearSegment(A);
    if (mask & 0b00000010) /*fillSegment(B, m_segmentColors[B]);*/{} else clearSegment(B);
    if (mask & 0b00000100) /*fillSegment(C, m_segmentColors[C]);*/{} else clearSegment(C);
    if (mask & 0b00001000) /*fillSegment(D, m_segmentColors[D]);*/{} else clearSegment(D);
    if (mask & 0b00010000) /*fillSegment(E, m_segmentColors[E]);*/{} else clearSegment(E);
    if (mask & 0b00100000) /*fillSegment(F, m_segmentColors[F]);*/{} else clearSegment(F);
    if (mask & 0b01000000) /*fillSegment(G, m_segmentColors[G]);*/{} else clearSegment(G);
}

/*
 One segment

   G
 A   F
   B
 C   E
   D

 */

void SevenSegmentDigit::fillSegment(Segment segment, const CRGB &color)
{
    size_t start = segment * m_ledsPerSegment;
    size_t end = start + m_ledsPerSegment;

    ESP_LOGD("SevenSegmentDigit", "fillSegment: start=%d, end=%d, color=rgb(%d, %d, %d)", start, end, color.r, color.g, color.b);
    std::fill(m_startLed + start, m_startLed + end, color);
}

std::string SevenSegmentDigit::toString() const
{
    char c = m_digit.value_or(' ');

    if (c == ' ') return "SevenSegmentDigit(digit= )";
    if (c == '\0') return "SevenSegmentDigit(digit=\\0)";

    return fmt::format("SevenSegmentDigit(digit={})", c);
}

void SevenSegmentDigit::forEverySegment(const std::function<void(Segment, CRGB*, CRGB*)>& func) const
{
    for (auto segment = Segment(0); segment <= LAST_SEGMENT; segment = Segment(segment + 1))
    {
        size_t start = segment * m_ledsPerSegment;
        size_t end = start + m_ledsPerSegment;

        CRGB* startLed = m_startLed + start;
        CRGB* endLed = m_startLed + end;

        func(segment, startLed, endLed);
    }
}
