#pragma once

// system includes
#include <expected>
#include <string>
#include <type_traits>

// esp-idf includes
#include <esp_sntp.h>

// 3rdparty lib includes
#include <ArduinoJson.h>
#include <color_utils.h>
#include <configwrapper.h>
#include <cpputils.h>
#include <espchrono.h>
#include <espwifistack.h>
#include <fmt/core.h>
#include <numberparsing.h>

// local includes
#include "utils/config.h"
#include "utils/typehelpers.h"

namespace webserver::apihelpers {

using namespace typeutils;

using FromJsonReturnType = std::expected<void, std::string>;

template<typename T>
typename std::enable_if<
        !std::is_same_v<T, bool> &&
        !std::is_integral_v<T> &&
        !std::is_floating_point_v<T> &&
        !std::is_same_v<T, std::string> &&
        !std::is_same_v<T, wifi_stack::ip_address_t> &&
        !std::is_same_v<T, wifi_stack::mac_t> &&
        !std::is_same_v<T, std::optional<wifi_stack::mac_t>> &&
        !std::is_same_v<T, wifi_auth_mode_t> &&
        !std::is_same_v<T, sntp_sync_mode_t> &&
        !std::is_same_v<T, espchrono::DayLightSavingMode> &&
        !typeutils::is_optional_v<T> &&
        !std::is_same_v<T, cpputils::ColorHelper> &&
        !std::is_same_v<T, SecondaryBrightnessMode> &&
        !std::is_same_v<T, LedAnimationName> &&
        !is_duration_v<T>
        , FromJsonReturnType>::type
fromJson(ConfigWrapper<T>& config, const std::string_view value)
{
    ESP_LOGW("fromJson", "fromJson not implemented for type %s (nvsKey=%s)", t_to_str<T>::str, config.nvsName());
    return std::unexpected(fmt::format("fromJson not implemented for type {} (nvsKey={})", t_to_str<T>::str, config.nvsName()));
}

template<typename T>
typename std::enable_if<
        std::is_same_v<T, SecondaryBrightnessMode> ||
        std::is_same_v<T, LedAnimationName>
        , FromJsonReturnType>::type
fromJson(ConfigWrapper<T>& config, const std::string_view value)
{
    if (const auto res = parseEnum<T>::parse(value); res.has_value())
        return configs.write_config(config, res.value());
    else
        return std::unexpected(fmt::format("Invalid value for {}: {} ({})", t_to_str<T>::str, value, res.error()));
}

template<typename T>
typename std::enable_if<
        std::is_same_v<T, espchrono::seconds32>
        , FromJsonReturnType>::type
fromJson(ConfigWrapper<T>& config, const std::string_view value)
{
    if (auto parsed = cpputils::fromString<int32_t>(value))
        return configs.write_config(config, espchrono::seconds32{*parsed});
    else
        return std::unexpected(fmt::format("Invalid value for duration: {}", value));
}

template<typename T>
typename std::enable_if<
        std::is_same_v<T, espchrono::minutes32>
        , FromJsonReturnType>::type
fromJson(ConfigWrapper<T>& config, const std::string_view value)
{
    if (auto parsed = cpputils::fromString<int32_t>(value))
        return configs.write_config(config, espchrono::minutes32{*parsed});
    else
        return std::unexpected(fmt::format("Invalid value for duration: {}", value));
}

template<typename T>
typename std::enable_if<
        std::is_same_v<T, espchrono::milliseconds32>
        , FromJsonReturnType>::type
fromJson(ConfigWrapper<T>& config, const std::string_view value)
{
    if (auto parsed = cpputils::fromString<int32_t>(value))
        return configs.write_config(config, espchrono::milliseconds32{*parsed});
    else
        return std::unexpected(fmt::format("Invalid value for duration: {}", value));
}

template<typename T>
typename std::enable_if<
        std::is_same_v<T, std::string>
        , FromJsonReturnType>::type
fromJson(ConfigWrapper<T>& config, const std::string_view value)
{
    return configs.write_config(config, std::string{value});
}

template<typename T>
typename std::enable_if<
        std::is_same_v<T, bool>
        , FromJsonReturnType>::type
fromJson(ConfigWrapper<T>& config, const std::string_view value)
{
    if (cpputils::is_in(value, "true", "false"))
        return configs.write_config(config, value == "true");
    else
        return std::unexpected(fmt::format("Invalid value for bool: {}", value));
}

template<typename T>
typename std::enable_if<
        (
            std::is_integral_v<T> ||
            std::is_floating_point_v<T>
        ) &&
        !std::is_same_v<T, bool>
        , FromJsonReturnType>::type
fromJson(ConfigWrapper<T>& config, const std::string_view value)
{
    if (auto parsed = cpputils::fromString<T>(value))
        return configs.write_config(config, *parsed);
    else
        return std::unexpected(fmt::format("Invalid value for integral: {}", value));
}

template<typename T>
typename std::enable_if<
        std::is_same_v<T, std::optional<wifi_stack::mac_t>>
        , FromJsonReturnType>::type
fromJson(ConfigWrapper<T>& config, const std::string_view value)
{
    if (value.empty() || value == "null")
        return configs.write_config(config, std::nullopt);
    else if (const auto parsed = wifi_stack::fromString<wifi_stack::mac_t>(value); parsed)
        return configs.write_config(config, *parsed);
    else
        return std::unexpected(parsed.error());
}

template<typename T>
typename std::enable_if<
        std::is_same_v<T, wifi_stack::ip_address_t>
        , FromJsonReturnType>::type
fromJson(ConfigWrapper<T>& config, const std::string_view value)
{
    if (const auto parsed = wifi_stack::fromString<wifi_stack::ip_address_t>(value); parsed)
        return configs.write_config(config, *parsed);
    else
        return std::unexpected(parsed.error());
}

template<typename T>
typename std::enable_if<
        std::is_same_v<T, wifi_stack::mac_t>
        , FromJsonReturnType>::type
fromJson(ConfigWrapper<T>& config, const std::string_view value)
{
    if (const auto parsed = wifi_stack::fromString<wifi_stack::mac_t>(value); parsed)
        return configs.write_config(config, *parsed);
    else
        return std::unexpected(parsed.error());
}

template<typename T>
typename std::enable_if<
        std::is_same_v<T, sntp_sync_mode_t> ||
        std::is_same_v<T, wifi_auth_mode_t> ||
        std::is_same_v<T, espchrono::DayLightSavingMode>
        , FromJsonReturnType>::type
fromJson(ConfigWrapper<T>& config, const std::string_view value)
{
    if (auto parsed = cpputils::fromString<std::underlying_type_t<T>>(value))
        return configs.write_config(config, T(*parsed));
    else
        return std::unexpected(fmt::format("could not parse {}", value));
}

template<typename T>
typename std::enable_if<
        std::is_same_v<T, cpputils::ColorHelper>
        , FromJsonReturnType>::type
fromJson(ConfigWrapper<T>& config, const std::string_view value)
{
    if (auto parsed = cpputils::parseColor(value))
        return configs.write_config(config, *parsed);
    else
        return std::unexpected(fmt::format("could not parse {}", value));
}

template<typename T>
typename std::enable_if<
        typeutils::is_optional_v<T> &&
        !std::is_same_v<T, std::optional<wifi_stack::mac_t>>
        , FromJsonReturnType>::type
fromJson(ConfigWrapper<T>& config, const std::string_view value)
{
    if (value.empty() || value == "null")
        return configs.write_config(config, std::nullopt);
    else
    {
        return fromJson(config, value);
    }
}

} // namespace webserver::apihelpers
