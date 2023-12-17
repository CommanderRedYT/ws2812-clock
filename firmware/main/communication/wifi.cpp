#include "wifi.h"

constexpr const char * const TAG = "WIFI";

// system includes
#include <expected>
#include <string>
#include <optional>

// esp-idf includes
#include <esp_log.h>

// 3rdparty lib includes
#include <espwifistack.h>
#include <recursivelockhelper.h>

// local includes
#include "utils/config.h"
#include "utils/espclock.h"
#include "utils/global_lock.h"

namespace wifi {

namespace {

wifi_stack::config createConfig();
std::optional<wifi_stack::sta_config> createStaConfig();
wifi_stack::wifi_entry createWiFiEntry(const WiFiConfig& wiFiConfig);
std::optional<wifi_stack::ap_config> createApConfig();

bool lastStaConnected{false};

} // namespace

void begin()
{
    ESP_LOGI(TAG, "begin");

    wifi_stack::init(createConfig());
}

void update()
{
    espcpputils::RecursiveLockHelper guard{global::global_lock->handle};
    wifi_stack::update(createConfig());

    const bool staConnected = isStaConnected();

    if (staConnected != lastStaConnected)
    {
        lastStaConnected = staConnected;
        if (staConnected)
        {
            ESP_LOGI(TAG, "STA connected");
            espclock::syncNow();
        }
        else
            ESP_LOGI(TAG, "STA disconnected");
    }
}

std::expected<void, std::string> startScan()
{
    const auto &staConfig = createStaConfig();
    if (!staConfig)
        return std::unexpected("WIFI_STA is not enabled");

    if (const auto result = wifi_stack::begin_scan(*staConfig); !result)
        return std::unexpected(fmt::format("wifi_stack::begin_scan() failed: {}", result.error()));

    return {};
}

bool isStaConnected()
{
    return wifi_stack::get_sta_status() == wifi_stack::WiFiStaStatus::CONNECTED;
}

bool isApUp()
{
    if (!configs.wifiApEnabled.value())
        return false;

    if (
            configs.wifiApOnlyWhenNotConnected.value() &&
            wifi_stack::get_sta_status() == wifi_stack::WiFiStaStatus::CONNECTED
    )
        return false;

    return true;
}

namespace {

wifi_stack::config createConfig()
{
    return wifi_stack::config {
        .base_mac_override = configs.baseMacAddressOverride.value(),
        .sta = createStaConfig(),
        .ap = createApConfig(),
    };
}

std::optional<wifi_stack::sta_config> createStaConfig()
{
    if (!configs.wifiStaEnabled.value())
        return std::nullopt;

    return wifi_stack::sta_config{
        .hostname = configs.hostname.value(),
        .wifis = []() {
            std::array<wifi_stack::wifi_entry, CONFIG_WIFI_STA_CONFIG_COUNT> wifis;
            for (size_t i = 0; i < CONFIG_WIFI_STA_CONFIG_COUNT; ++i)
                wifis[i] = createWiFiEntry(configs.wifis[i]);
            return wifis;
        }(),
        .min_rssi = -90,
        .long_range = false,
    };
}

wifi_stack::wifi_entry createWiFiEntry(const WiFiConfig& wiFiConfig)
{
    std::optional<wifi_stack::static_ip_config> staticIpConfig;

    if (wiFiConfig.useStaticIp.value())
    {
        staticIpConfig = wifi_stack::static_ip_config{
            .ip = wiFiConfig.staticIp.value(),
            .subnet = wiFiConfig.staticSubnet.value(),
            .gateway = wiFiConfig.staticGateway.value(),
        };
    }

    wifi_stack::static_dns_config staticDnsConfig;

    if (wiFiConfig.useStaticDns.value())
    {
        if (wiFiConfig.staticDns0.value().value())
            staticDnsConfig.main = wiFiConfig.staticDns0.value();
        if (wiFiConfig.staticDns1.value().value())
            staticDnsConfig.backup = wiFiConfig.staticDns1.value();
        if (wiFiConfig.staticDns2.value().value())
            staticDnsConfig.fallback = wiFiConfig.staticDns2.value();
    }

    return wifi_stack::wifi_entry{
        .ssid = wiFiConfig.ssid.value(),
        .key = wiFiConfig.key.value(),
        .static_ip = staticIpConfig,
        .static_dns = staticDnsConfig,
    };
}

std::optional<wifi_stack::ap_config> createApConfig()
{
    if (!isApUp())
        return std::nullopt;

    return wifi_stack::ap_config {
        .hostname = configs.hostname.value(),
        .ssid = configs.wifiApName.value(),
        .key = configs.wifiApKey.value(),
        .static_ip = {
                .ip = configs.wifiApIp.value(),
                .subnet = configs.wifiApMask.value(),
                .gateway = configs.wifiApIp.value()
        },
        .channel = configs.wifiApChannel.value(),
        .authmode = configs.wifiApAuthmode.value(),
        .ssid_hidden = false,
        .max_connection = 4,
        .beacon_interval = 100,
        .long_range = false
    };
}

} // namespace

} // namespace wifi
