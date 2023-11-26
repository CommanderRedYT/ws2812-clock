#pragma once

// system includes
#include <expected>

// local includes
#include "fromJson.h"

namespace webserver {

template<typename T>
std::expected<void, std::string> saveSetting(ConfigWrapper<T> &config, const std::string_view newValue)
{
    ESP_LOGI("ConfigApiHelper", "%s=%s", config.nvsName(), newValue.data());
    if (auto parsed = apihelpers::fromJson(config, newValue))
    {
        return {};
    }
    else
    {
        return parsed;
    }
}

} // namespace webserver
