#include "status.h"

// 3rdparty lib includes
#include <ArduinoJson.h>
#include <espchrono.h>
#include <espwifistack.h>

// local includes
#include "communication/webserver_api.h"
#include "peripheral/bme280.h"
#include "peripheral/ledhelpers/ledanimation.h"
#include "peripheral/ledmanager.h"
#include "utils/config.h"
#include "utils/espclock.h"

using namespace webserver;
using namespace std::chrono_literals;

namespace status {

namespace {
double round2(double value) {
    return (int)(value * 100 + 0.5) / 100.0;
}
} // namespace

esp_err_t generateStatusJson(JsonObject& statusObj)
{
    statusObj["version"] = VERSION;

    {
        statusObj["name"] = configs.customName.value();
    }

    {
        auto timeObj = statusObj.createNestedObject("time");

        timeObj["millis"] = espchrono::millis_clock::now().time_since_epoch() / 1ms;
        timeObj["local"] = toString(toDateTime(espchrono::local_clock::now()));
        timeObj["utc"] = toString(toDateTime(espchrono::utc_clock::now()));
        timeObj["offset"] = espchrono::local_clock::now().timezone.offset.count();
        timeObj["dst"] = toString(espchrono::local_clock::now().timezone.dayLightSavingMode);
        timeObj["synced"] = espclock::isSynced();
        if (const auto res = espclock::sunrise(); res)
            timeObj["sunrise"] = toString(toDateTime(const_cast<espchrono::utc_clock::time_point&>(*res)));
        else
            timeObj["sunrise"] = nullptr;

        if (const auto res = espclock::sunset(); res)
            timeObj["sunset"] = toString(toDateTime(const_cast<espchrono::utc_clock::time_point&>(*res)));
        else
            timeObj["sunset"] = nullptr;
        timeObj["night"] = espclock::isNight();
    }

    {
        using namespace ledmanager;

        auto ledObj = statusObj.createNestedObject("led");

        if (!ledManager)
        {
            ledObj["error"] = "ledManager is not initialized";
        }
        else
        {
            ledObj["fps"] = ledManager->getFps();
            ledObj["brightness"] = ledManager->getBrightness();
            ledObj["visible"] = ledManager->isVisible();
            if (animation::currentAnimation)
                if (auto enumValue = animation::currentAnimation->getEnumValue(); enumValue)
                    ledObj["animation"] = toString(*enumValue);
                else
                    ledObj["animation"] = nullptr;
            else
                ledObj["animation"] = nullptr;

            JsonObject hassObj = ledObj.createNestedObject("homeassistant");
            hassObj["brightness"] = configs.ledBrightness.value();
            hassObj["state"] = configs.ledAnimationEnabled.value() ? "ON" : "OFF";
            if (animation::currentAnimation)
                if (auto enumValue = animation::currentAnimation->getEnumValue(); enumValue)
                    ledObj["effect"] = toString(*enumValue);
                else
                    ledObj["effect"] = nullptr;
            else
                ledObj["effect"] = nullptr;

            auto &primaryColor = configs.primaryColor.value();
            JsonObject colorObj = hassObj.createNestedObject("color");
            colorObj["r"] = primaryColor.r;
            colorObj["g"] = primaryColor.g;
            colorObj["b"] = primaryColor.b;
        }
    }

    {
        auto bme280obj = statusObj.createNestedObject("bme280");

        if (const auto res = bme280_sensor::getData(); !res)
        {
            bme280obj["error"] = res.error();
        }
        else
        {
            bme280obj["temp"] = round2(res->temperature);
            bme280obj["pressure"] = round2(res->pressure);
            bme280obj["humidity"] = round2(res->humidity);
            bme280obj["altitude"] = round2(res->altitude);
            bme280obj["millis_ts"] = res->timestamp.time_since_epoch() / 1ms;
        }
    }

    {
        auto staObj = statusObj.createNestedObject("sta");

        if (const auto ap_result = wifi_stack::get_sta_ap_info(); ap_result)
        {
            staObj["ssid"] = ap_result->ssid;
            staObj["rssi"] = ap_result->rssi;
            staObj["channel"] = ap_result->primary;
            staObj["bssid"] = wifi_stack::toString(wifi_stack::mac_t{ap_result->bssid});
            staObj["auth"] = wifi_stack::toString(ap_result->authmode);

            if (const auto ip_result = wifi_stack::get_ip_info(wifi_stack::esp_netifs[ESP_IF_WIFI_STA]))
            {
                staObj["ip"] = wifi_stack::toString(ip_result->ip);
                staObj["mask"] = wifi_stack::toString(ip_result->netmask);
                staObj["gateway"] = wifi_stack::toString(ip_result->gw);
            }
            else
            {
                staObj["error"] = ip_result.error();
            }
        }
        else
        {
            staObj["error"] = ap_result.error();
        }
    }

    return ESP_OK;
}

esp_err_t generateStatusJson(JsonDocument& statusObj)
{
    statusObj.clear();

    auto dummy = statusObj.to<JsonObject>();

    return generateStatusJson(dummy);
}

esp_err_t generateStatusJson(std::string& json)
{
    auto guard = getApiJson();
    auto& doc = *guard;

    doc.clear();

    doc["success"] = true;

    auto statusObj = doc.createNestedObject("status");

    generateStatusJson(statusObj);

    if (doc.overflowed())
    {
        return ESP_FAIL;
    }

    serializeJson(doc, json);

    return ESP_OK;
}

esp_err_t forEveryKey(const std::function<void(const JsonString&, const JsonVariant&)>& callback)
{
    auto guard = getApiJson();
    auto& doc = *guard;

    doc.clear();

    generateStatusJson(doc);

    for (const auto& kv : doc.as<JsonObject>())
    {
        if (kv.value().is<JsonObject>())
        {
            for (const auto& kv2 : kv.value().as<JsonObject>())
            {
                // kv.key()/kv2.key()
                std::string key = fmt::format("{}/{}", kv.key().c_str(), kv2.key().c_str());
                callback(key.c_str(), kv2.value());
            }
        }
        else
        {
            callback(kv.key(), kv.value());
        }
    }

    return ESP_OK;
}

} // namespace status
