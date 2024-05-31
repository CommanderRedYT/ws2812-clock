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
#include <espchrono.h>
#include <espwifistack.h>

// local includes
#include "utils/typehelpers.h"

namespace webserver::apihelpers {

using namespace typeutils;

using FromJsonReturnType = std::expected<void, std::string>;

template<typename T>
typename std::enable_if<
        !std::is_same_v<T, bool> &&
        !std::is_integral_v<T> &&
        !std::is_floating_point_v<T> &&
        !is_duration_v<T> &&
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
        !std::is_same_v<T, LedAnimationName>
        , FromJsonReturnType>::type
toJson(const T& value, JsonDocument &doc)
{
    ESP_LOGW("toJson", "toJson not implemented for type %s", t_to_str<T>::str);
    return std::unexpected(std::string("toJson not implemented for type ") + t_to_str<T>::str);
}

template<typename T>
typename std::enable_if<
        !is_duration_v<T> &&
        (std::is_same_v<T, SecondaryBrightnessMode> ||
         std::is_same_v<T, LedAnimationName>)
        , FromJsonReturnType>::type
toJson(const T& value, JsonDocument &doc)
{
    doc["value"] = toString(value);
    JsonArray arr = doc.createNestedArray("values");

    typesafeenum::iterateEnum<T>::iterate([&](T enum_value, const auto &string_value) {
        arr.add(string_value);
    });

    return {};
}

template<typename T>
typename std::enable_if<
        std::is_same_v<T, bool> ||
        std::is_integral_v<T> ||
        std::is_floating_point_v<T>
        , FromJsonReturnType>::type
toJson(const T& value, JsonDocument &doc)
{
    doc.set(value);
    return {};
}

template<typename T>
typename std::enable_if<
        std::is_same_v<T, std::string>
        , FromJsonReturnType>::type
toJson(const T& value, JsonDocument &doc)
{
    doc.set(value);
    return {};
}

template<typename T>
typename std::enable_if<
        std::is_same_v<T, std::optional<wifi_stack::mac_t>>
        , FromJsonReturnType>::type
toJson(const T& value, JsonDocument &doc)
{
    if (value.has_value())
    {
        doc.set(toString(value.value()));
    }
    else
    {
        doc.set(nullptr);
    }
    return {};
}

template<typename T>
typename std::enable_if<
        std::is_same_v<T, wifi_auth_mode_t> ||
        std::is_same_v<T, wifi_stack::ip_address_t> ||
        std::is_same_v<T, wifi_stack::mac_t>
        , FromJsonReturnType>::type
toJson(const T& value, JsonDocument &doc)
{
    doc.set(wifi_stack::toString(value));
    return {};
}

template<typename T>
typename std::enable_if<
        std::is_same_v<T, sntp_sync_mode_t> ||
        std::is_same_v<T, espchrono::DayLightSavingMode>
        , FromJsonReturnType>::type
toJson(const T& value, JsonDocument &doc)
{
    doc.set(std::to_underlying(value));
    return {};
}

template<typename T>
typename std::enable_if<
        typeutils::is_duration_v<T>
        , FromJsonReturnType>::type
toJson(const T& value, JsonDocument &doc)
{
    doc.set(value / 1ms);
    return {};
}

template<typename T>
typename std::enable_if<
        std::is_same_v<T, cpputils::ColorHelper>
        , FromJsonReturnType>::type
toJson(const T& value, JsonDocument &doc)
{
    doc.set(cpputils::toString(value));
    return {};
}

template<typename T>
typename std::enable_if<
        typeutils::is_optional_v<T> &&
        !std::is_same_v<T, std::optional<wifi_stack::mac_t>>
        , FromJsonReturnType>::type
toJson(const T& value, JsonDocument &doc)
{
    if (value.has_value())
    {
        if (const auto res = toJson(value.value(), doc); !res)
            return res;
    }
    else
    {
        doc.set(nullptr);
    }
    return {};
}

} // namespace webserver::apihelpers
