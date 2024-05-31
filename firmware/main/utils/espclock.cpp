#include "espclock.h"

constexpr const char * const TAG = "espclock";

// esp-idf includes
#include <apps/esp_sntp.h>
#include <esp_ota_ops.h>

// 3rdparty lib includes
#include <espchrono.h>
#include <recursivelockhelper.h>
#include <sunset.h>

// local includes
#include "peripheral/ledhelpers/ledanimation.h"
#include "peripheral/ledmanager.h"
#include "utils/config.h"

espchrono::time_zone get_default_timezone() noexcept
{
    using namespace espchrono;
    return time_zone{configs.timeOffset.value(), configs.timeDst.value()};
}

#ifdef CONFIG_ESPCHRONO_SUPPORT_DEFAULT_TIMEZONE
espchrono::time_zone espchrono::get_default_timezone() noexcept
{
    return ::get_default_timezone();
}
#endif // CONFIG_ESPCHRONO_SUPPORT_DEFAULT_TIMEZONE

namespace espclock {

std::optional<espchrono::utc_clock::time_point> sunrise_time;
std::optional<espchrono::utc_clock::time_point> sunset_time;
std::optional<espchrono::millis_clock::time_point> last_sync_time;

void update_sunrise_sunset()
{
    static SunSet sunSet;

    if (!isSynced())
        return;

    if (!last_sync_time || espchrono::ago(*last_sync_time) > 1h)
    {
        sunSet.setPosition(configs.sunriseLatitude.value(), configs.sunriseLongitude.value(), 0);

        const auto local_now = espchrono::local_clock::now();
        const auto now_dt = espchrono::toDateTime(local_now);
        sunSet.setCurrentDate(static_cast<int>(now_dt.date.year()), static_cast<unsigned int>(now_dt.date.month()),
                              static_cast<unsigned int>(now_dt.date.day()));

        // sunSet.calcSunrise() => int (minutes past midnight)
        const auto midnight = std::chrono::floor<std::chrono::days>(local_now);
        const auto sunrise_minutes = std::chrono::minutes{static_cast<int>(sunSet.calcSunrise())};
        const auto sunset_minutes = std::chrono::minutes{static_cast<int>(sunSet.calcSunset())};
        sunrise_time = espchrono::utc_clock::time_point{midnight.time_since_epoch() + sunrise_minutes};
        sunset_time = espchrono::utc_clock::time_point{midnight.time_since_epoch() + sunset_minutes};

        ESP_LOGI(TAG, "Sunrise: %lld", sunrise_time->time_since_epoch() / 1s);
        ESP_LOGI(TAG, "Sunset: %lld", sunset_time->time_since_epoch() / 1s);

        last_sync_time = espchrono::millis_clock::now();
    }
}

const std::optional<espchrono::utc_clock::time_point>& sunrise()
{
    update_sunrise_sunset();
    return sunrise_time;
}

const std::optional<espchrono::utc_clock::time_point>& sunset()
{
    update_sunrise_sunset();
    return sunset_time;
}

bool isNight()
{
    if (!isSynced())
        return false;

    const auto now = espchrono::utc_clock::now();
    return now < sunrise().value() || now > sunset().value();
}

namespace {
bool time_synced{false};
bool time_synced_prev{false};

void time_sync_notification_cb(struct timeval *tv)
{
    if (tv == nullptr)
    {
        ESP_LOGE(TAG, "Time sync notification with nullptr");
        return;
    }

    ESP_LOGI(TAG, "Time sync notification (%lld)", tv->tv_sec);

    if (tv->tv_sec > 1577836800)
        time_synced = true;
}

} // namespace

void setTimeInLedManager()
{
    if (ledmanager::ledManager && configs.ledOverrideDigits.value().empty())
    {
        auto& ledManager = *ledmanager::ledManager;

        auto now = espchrono::toDateTime(espchrono::local_clock::now());

        auto hour = now.hour;
        auto minute = now.minute;

        // 0 1 2 3
        // h h m m

        if (const auto animation = animation::currentAnimation; animation && animation->shouldSetDigits())
        {
            ledManager.digits[0].setDigit(hour / 10);
            ledManager.digits[1].setDigit(hour % 10);
            ledManager.digits[2].setDigit(minute / 10);
            ledManager.digits[3].setDigit(minute % 10);
        }
    }
}

void begin()
{
    // init sntp

    esp_sntp_setoperatingmode(ESP_SNTP_OPMODE_POLL);
    static_assert(SNTP_MAX_SERVERS >= 1);

    esp_sntp_setservername(0, configs.timeServer.value().c_str());
    esp_sntp_set_time_sync_notification_cb(time_sync_notification_cb);
    esp_sntp_set_sync_mode(configs.timeSyncMode.value());
    esp_sntp_set_sync_interval(espchrono::milliseconds32{configs.timeSyncInterval.value()}.count());

    esp_sntp_init();

    if (!esp_sntp_enabled())
    {
        ESP_LOGE(TAG, "esp_sntp_init() failed");
    }
}

void update()
{
    setTimeInLedManager();
}

void syncNow()
{
    if (!esp_sntp_enabled())
    {
        ESP_LOGE(TAG, "esp_sntp not enabled");
        return;
    }

    esp_sntp_restart();
}

bool isSynced()
{
    if (time_synced_prev != time_synced && time_synced)
    {
        ESP_LOGI(TAG, "Time synced, marking app as valid");
        esp_ota_mark_app_valid_cancel_rollback();
        time_synced_prev = time_synced;
    }

    return time_synced;
}

} // namespace espclock
