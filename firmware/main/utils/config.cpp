#include "config.h"

// system includes
#include <string>

// 3rdparty lib includes
#include <configmanager_priv.h>
#include <espwifistack.h>
#include <fmt/core.h>

std::string defaultHostname()
{
    if (const auto result = wifi_stack::get_default_mac_addr())
        return fmt::format("ws2812-clock_{:02x}{:02x}{:02x}", result->at(3), result->at(4), result->at(5));
    else
        ESP_LOGE(config::TAG, "get_default_mac_addr() failed: %.*s", result.error().size(), result.error().data());
    return "ws2812-clock";
}

ConfigManager<ConfigContainer> configs;

INSTANTIATE_CONFIGMANAGER_TEMPLATES(ConfigContainer)
