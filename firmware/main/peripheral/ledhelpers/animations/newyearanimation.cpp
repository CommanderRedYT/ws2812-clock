#include "newyearanimation.h"

// local includes
#include "peripheral/ledmanager.h"

namespace animation {

void NewYearAnimation::update()
{
    Base::update();

    espchrono::local_time_point local_now = espchrono::local_clock::now();
    auto dateTime = toDateTime(local_now);
    dateTime.hour = 23;
    dateTime.minute = 59;
    dateTime.second = 59;
    date::year_month_day silvesterDate{date::year(dateTime.date.year()), date::month(12), date::day(31)};
    dateTime.date = silvesterDate;

    espchrono::local_clock::time_point silvester = fromDateTime(dateTime);

    const auto diff = silvester - espchrono::local_clock::now();
    const auto diff_hours = std::chrono::duration_cast<std::chrono::hours>(diff).count() % 24;
    const auto diff_minutes = std::chrono::duration_cast<std::chrono::minutes>(diff).count() % 60;
    const auto diff_seconds = std::chrono::duration_cast<std::chrono::seconds>(diff).count() % 60;

    if (ledmanager::ledManager)
    {
        auto& ledManager = *ledmanager::ledManager;

        if (diff_hours == 0)
        {
            ledManager.digits[0].setDigit(diff_minutes / 10);
            ledManager.digits[1].setDigit(diff_minutes % 10);
            ledManager.digits[2].setDigit(diff_seconds / 10);
            ledManager.digits[3].setDigit(diff_seconds % 10);
        }
        else
        {
            ledManager.digits[0].setDigit(diff_hours / 10);
            ledManager.digits[1].setDigit(diff_hours % 10);
            ledManager.digits[2].setDigit(diff_minutes / 10);
            ledManager.digits[3].setDigit(diff_minutes % 10);
        }
    }

    m_hue = static_cast<uint8_t>(diff_seconds * 4); // 4 = 256 / 60
}

void NewYearAnimation::render_all(CRGB* leds, size_t leds_length)
{
    Base::render_all(leds, leds_length);

    fill_rainbow(leds, leds_length, m_hue, 1);
}


} // namespace animation
