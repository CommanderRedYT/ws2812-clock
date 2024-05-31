#include "clockdot.h"

// system includes
#include <algorithm>

// local includes
#include "utils/config.h"

ClockDot::ClockDot(DotPlacement placement, CRGB* startLed, size_t length)
    : m_startLed{startLed}, m_length{length}, m_on{false}, m_placement{placement}
{}

void ClockDot::render()
{
    if (!m_on || !configs.ledOverrideDigits.value().empty())
        std::fill(m_startLed, m_startLed + m_length, CRGB::Black);
}

std::string ClockDot::toString() const
{
    return m_on ? "On" : "Off";
}
