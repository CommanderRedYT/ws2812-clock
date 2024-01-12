#include "otaanimation.h"

// local includes
#include "utils/config.h"
#include "communication/ota.h"

namespace animation::internal {

void OtaAnimation::update()
{
    ESP_LOGI("OtaAnimation", "update");
    Base::update();

    auto otaPercentage = static_cast<uint8_t>(ota::percent());

    m_percentage[0] = otaPercentage / 100;
    m_percentage[1] = (otaPercentage / 10) % 10;
    m_percentage[2] = otaPercentage % 10;

    m_digit_count = otaPercentage == 100 ? 3 : otaPercentage / 10 == 0 ? 1 : 2;
}

void OtaAnimation::render_digit(SevenSegmentDigit& sevenSegmentDigit, size_t digit_idx, CRGB* leds, size_t length)
{
    ESP_LOGI("OtaAnimation", "render_digit digit_idx=%d", digit_idx);
    Base::render_digit(sevenSegmentDigit, digit_idx, leds, length);

    if (digit_idx > 0 && digit_idx <= 3 && 4 - m_digit_count <= digit_idx)
    {
        sevenSegmentDigit.setDigit(m_percentage[digit_idx-1]);
    }
    else
    {
        sevenSegmentDigit.clear();
    }

    std::fill(leds, leds + length, CRGB::Green);
}

void OtaAnimation::render_dot(ClockDot& clockDot, CRGB* leds, size_t leds_length)
{
    std::fill(clockDot.begin(), clockDot.end(), CRGB::Black);
}

LedAnimation* otaAnimation{new OtaAnimation()};

} // namespace animation::internal
