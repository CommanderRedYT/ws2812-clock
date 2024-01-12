#include "mdns_clock.h"

constexpr const char * const TAG = "mDNS";

// esp-idf includes
#include <mdns.h>
#include <esp_log.h>

// 3rdparty lib includes
#include <recursivelockhelper.h>

// local includes
#include "utils/config.h"
#include "utils/global_lock.h"

namespace mdns {

namespace {

bool initialized{};

std::string lastHostname;
std::string lastName;

} // namespace

void begin()
{
    initialized = false;

    if (const auto res = mdns_init(); res != ESP_OK)
    {
        ESP_LOGE(TAG, "mdns_init() failed with: %s", esp_err_to_name(res));
        return;
    }

    const auto& hostname = configs.hostname.value();
    if (const auto res = mdns_hostname_set(hostname.c_str()); res != ESP_OK)
    {
        ESP_LOGE(TAG, "mdns_hostname_set() \"%.*s\" failed with: %s", hostname.size(), hostname.data(), esp_err_to_name(res));
        return;
    }

    const auto& name = configs.customName.value();
    if (const auto res = mdns_instance_name_set(name.c_str()); res != ESP_OK)
    {
        ESP_LOGE(TAG, "mdns_instance_name_set() \"%.*s\" failed with: %s", name.size(), name.data(), esp_err_to_name(res));
        return;
    }

    lastHostname = hostname;
    lastName = name;

    mdns_txt_item_t txt_records[] {
        mdns_txt_item_t { .key = "name", .value = name.c_str() },
        mdns_txt_item_t { .key = "hostname", .value = hostname.c_str() },
    };

    if (const auto res = mdns_service_add(nullptr, "_http", "_tcp", 80, txt_records, sizeof(txt_records) / sizeof(mdns_txt_item_t)); res != ESP_OK)
    {
        ESP_LOGE(TAG, "mdns_service_add() failed with: %s", esp_err_to_name(res));
        return;
    }

    if (const auto res = mdns_service_instance_name_set("_http", "_tcp", name.c_str()); res != ESP_OK)
    {
        ESP_LOGE(TAG, "mdns_service_instance_name_set() failed with: %s", esp_err_to_name(res));
        return;
    }

    initialized = true;
}

void update()
{
    if (!initialized)
        return;

    espcpputils::RecursiveLockHelper guard{global::global_lock->handle};

    if (const auto& currentHostname = configs.hostname.value(); lastHostname != currentHostname)
    {
        if (const auto res = mdns_hostname_set(currentHostname.c_str()); res != ESP_OK)
        {
            ESP_LOGE(TAG, "mdns_hostname_set() \"%.*s\" failed with: %s", currentHostname.size(), currentHostname.data(), esp_err_to_name(res));
            return;
        }

        lastHostname = currentHostname;
    }

    if (const auto& currentName = configs.customName.value(); lastName != currentName)
    {
        if (const auto res = mdns_instance_name_set(currentName.c_str()); res != ESP_OK)
        {
            ESP_LOGE(TAG, "mdns_instance_name_set() \"%.*s\" failed with: %s", currentName.size(), currentName.data(), esp_err_to_name(res));
            return;
        }

        lastName = currentName;
    }
}

} // namespace mdns
