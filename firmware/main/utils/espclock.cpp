#include "espclock.h"

constexpr const char * const TAG = "espclock";

// esp-idf includes
#include <apps/esp_sntp.h>

// 3rdparty lib includes
#include <espchrono.h>
#include <recursivelockhelper.h>

// local includes
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

namespace {
bool time_synced{false};

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
    if (ledmanager::ledManager)
    {
        auto& ledManager = *ledmanager::ledManager;

        auto now = espchrono::toDateTime(espchrono::local_clock::now());

        auto hour = now.hour;
        auto minute = now.minute;

        // 0 1 2 3
        // h h m m

        ledManager.digits[0].setChar('0' + hour / 10);
        ledManager.digits[1].setChar('0' + hour % 10);
        ledManager.digits[2].setChar('0' + minute / 10);
        ledManager.digits[3].setChar('0' + minute % 10);
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
    return time_synced;
}

} // namespace espclock
