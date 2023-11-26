#pragma once

#include "sdkconfig.h"

// system includes
#include <array>
#include <optional>
#include <string>

// esp-idf includes
#include <esp_sntp.h>

// 3rdparty lib includes
#include <color_utils.h>
#include <configconstraints_base.h>
#include <configconstraints_espchrono.h>
#include <configmanager.h>
#include <configwrapper.h>
#include <espchrono.h>
#include <espwifistack.h>
#include <fmt/core.h>

// local includes
#include "peripheral/ledhelpers/ledanimation.h"

using namespace espconfig;
using namespace std::chrono_literals;

namespace config {
constexpr const char * const TAG = "config";
} // namespace

std::string defaultHostname();

enum class AllowReset { NoReset, DoReset };
constexpr auto NoReset = AllowReset::NoReset;
constexpr auto DoReset = AllowReset::DoReset;

template<typename T>
class ConfigWrapperDynamicKey : public ConfigWrapper<T>
{
    CPP_DISABLE_COPY_MOVE(ConfigWrapperDynamicKey);
    using Base = ConfigWrapper<T>;

public:
    using value_t = typename Base::value_t;
    using ConstraintCallback = typename Base::ConstraintCallback;

    explicit ConfigWrapperDynamicKey(const char* nvsKey) : m_nvsKey{nvsKey} {}

    const char* nvsName() const override { return m_nvsKey; }

private:
    const char* m_nvsKey;
};

class WiFiConfig
{
    using ip_address_t = wifi_stack::ip_address_t;
public:
    WiFiConfig(const char *ssidNvsKey, const char *keyNvsKey,
               const char *useStaticIpKey, const char *staticIpKey, const char *staticSubnetKey, const char *staticGatewayKey,
               const char *useStaticDnsKey, const char *staticDns0Key, const char *staticDns1Key, const char *staticDns2Key) :
            ssid         {ssidNvsKey      },
            key          {keyNvsKey       },
            useStaticIp  {useStaticIpKey  },
            staticIp     {staticIpKey     },
            staticSubnet {staticSubnetKey },
            staticGateway{staticGatewayKey},
            useStaticDns {useStaticDnsKey },
            staticDns0   {staticDns0Key   },
            staticDns1   {staticDns1Key   },
            staticDns2   {staticDns2Key   }
    {}

    struct : ConfigWrapperDynamicKey<std::string>
    {
        using ConfigWrapperDynamicKey<std::string>::ConfigWrapperDynamicKey;

        bool allowReset() const final { return true; }
        std::string defaultValue() const final { return {}; }
        ConfigConstraintReturnType checkValue(value_t value) const final { return StringMaxSize<32>(value); }
    } ssid;
    struct : ConfigWrapperDynamicKey<std::string>
    {
        using ConfigWrapperDynamicKey<std::string>::ConfigWrapperDynamicKey;

        bool allowReset() const final { return true; }
        std::string defaultValue() const final { return {}; }
        ConfigConstraintReturnType checkValue(value_t value) const final { return StringOr<StringEmpty, StringMinMaxSize<8, 64>>(value); }
    } key;
    struct : ConfigWrapperDynamicKey<bool>
    {
        using ConfigWrapperDynamicKey<bool>::ConfigWrapperDynamicKey;

        bool allowReset() const final { return true; }
        value_t defaultValue() const final { return false; }
        ConfigConstraintReturnType checkValue(value_t value) const final { return {}; }
    } useStaticIp;
    struct : ConfigWrapperDynamicKey<ip_address_t>
    {
        using ConfigWrapperDynamicKey<ip_address_t>::ConfigWrapperDynamicKey;

        bool allowReset() const final { return true; }
        value_t defaultValue() const final { return {}; }
        ConfigConstraintReturnType checkValue(value_t value) const final { return {}; }
    } staticIp;
    struct : ConfigWrapperDynamicKey<ip_address_t>
    {
        using ConfigWrapperDynamicKey<ip_address_t>::ConfigWrapperDynamicKey;

        bool allowReset() const final { return true; }
        value_t defaultValue() const final { return {}; }
        ConfigConstraintReturnType checkValue(value_t value) const final { return {}; }
    } staticSubnet;
    struct : ConfigWrapperDynamicKey<ip_address_t>
    {
        using ConfigWrapperDynamicKey<ip_address_t>::ConfigWrapperDynamicKey;

        bool allowReset() const final { return true; }
        value_t defaultValue() const final { return {}; }
        ConfigConstraintReturnType checkValue(value_t value) const final { return {}; }
    } staticGateway;
    struct : ConfigWrapperDynamicKey<bool>
    {
        using ConfigWrapperDynamicKey<bool>::ConfigWrapperDynamicKey;

        bool allowReset() const final { return true; }
        value_t defaultValue() const final { return false; }
        ConfigConstraintReturnType checkValue(value_t value) const final { return {}; }
    } useStaticDns;
    struct : ConfigWrapperDynamicKey<ip_address_t>
    {
        using ConfigWrapperDynamicKey<ip_address_t>::ConfigWrapperDynamicKey;

        bool allowReset() const final { return true; }
        value_t defaultValue() const final { return {}; }
        ConfigConstraintReturnType checkValue(value_t value) const final { return {}; }
    } staticDns0;
    struct : ConfigWrapperDynamicKey<ip_address_t>
    {
        using ConfigWrapperDynamicKey<ip_address_t>::ConfigWrapperDynamicKey;

        bool allowReset() const final { return true; }
        value_t defaultValue() const final { return {}; }
        ConfigConstraintReturnType checkValue(value_t value) const final { return {}; }
    } staticDns1;
    struct : ConfigWrapperDynamicKey<ip_address_t>
    {
        using ConfigWrapperDynamicKey<ip_address_t>::ConfigWrapperDynamicKey;

        bool allowReset() const final { return true; }
        value_t defaultValue() const final { return {}; }
        ConfigConstraintReturnType checkValue(value_t value) const final { return {}; }
    } staticDns2;
};

struct TimeRangeConfigWrapper
{
    TimeRangeConfigWrapper(const char *startNvsKey, const char *endNvsKey) : start{startNvsKey}, end{endNvsKey} {}

    using seconds32 = espchrono::seconds32;

    struct : ConfigWrapperDynamicKey<seconds32>
    {
        using ConfigWrapperDynamicKey<seconds32>::ConfigWrapperDynamicKey;

        bool allowReset() const final { return true; }
        value_t defaultValue() const final { return {}; }
        ConfigConstraintReturnType checkValue(value_t value) const final
        {
            if (value < 0s || value > 24h) {
                return std::unexpected("TimeRange::start should be positive and below 24h");
            }

            return {};
        }
    } start;
    struct : ConfigWrapperDynamicKey<seconds32>
    {
        using ConfigWrapperDynamicKey<seconds32>::ConfigWrapperDynamicKey;

        bool allowReset() const final { return true; }
        value_t defaultValue() const final { return {}; }
        ConfigConstraintReturnType checkValue(value_t value) const final
        {
            if (value < 0s || value > 24h) {
                return std::unexpected("TimeRange::start should be positive and below 24h");
            }

            return {};
        }
    } end;
};

class ConfigContainer;

extern ConfigManager<ConfigContainer> configs;

class ConfigContainer
{
    using DayLightSavingMode = espchrono::DayLightSavingMode;
    using hours32 = espchrono::hours32;
    using mac_t = wifi_stack::mac_t;
    using milliseconds32 = espchrono::milliseconds32;
    using minutes32 = espchrono::minutes32;
    using seconds32 = espchrono::seconds32;
    using ColorHelper = cpputils::ColorHelper;
public:
    // Connectivity
    struct : ConfigWrapper<std::string>
    {
        bool allowReset() const final { return true; }
        const char* nvsName() const final { return "hostname"; }
        std::string defaultValue() const final { return defaultHostname(); }
        ConfigConstraintReturnType checkValue(value_t value) const final { return StringMaxSize<32>(value); }
    } hostname;

    struct : ConfigWrapper<std::string>
    {
        bool allowReset() const final { return true; }
        const char* nvsName() const final { return "customName"; }
        std::string defaultValue() const final { return defaultHostname(); }
        ConfigConstraintReturnType checkValue(value_t value) const final { return StringMaxSize<32>(value); }
    } name;

    struct : ConfigWrapper<std::optional<mac_t>>
    {
        bool allowReset() const final { return false; }
        const char *nvsName() const final { return "baseMacAddrOver"; }
        value_t defaultValue() const final { return std::nullopt; }
        ConfigConstraintReturnType checkValue(value_t value) const final { return {}; }
    } baseMacAddressOverride;

    struct : ConfigWrapper<bool>
    {
        bool allowReset() const final { return true; }
        const char *nvsName() const final { return "wifi_enabled"; }
        value_t defaultValue() const final { return true; }
        ConfigConstraintReturnType checkValue(value_t value) const final { return {}; }
    } wifiStaEnabled;

    std::array<WiFiConfig, 10> wifis {
        WiFiConfig{"wifi_ssid0", "wifi_key0", "wifi_usestatic0", "wifi_static_ip0", "wifi_stati_sub0", "wifi_stat_gate0", "wifi_usestadns0", "wifi_stat_dnsA0", "wifi_stat_dnsB0", "wifi_stat_dnsC0"},
        WiFiConfig{"wifi_ssid1", "wifi_key1", "wifi_usestatic1", "wifi_static_ip1", "wifi_stati_sub1", "wifi_stat_gate1", "wifi_usestadns1", "wifi_stat_dnsA1", "wifi_stat_dnsB1", "wifi_stat_dnsC1"},
        WiFiConfig{"wifi_ssid2", "wifi_key2", "wifi_usestatic2", "wifi_static_ip2", "wifi_stati_sub2", "wifi_stat_gate2", "wifi_usestadns2", "wifi_stat_dnsA2", "wifi_stat_dnsB2", "wifi_stat_dnsC2"},
        WiFiConfig{"wifi_ssid3", "wifi_key3", "wifi_usestatic3", "wifi_static_ip3", "wifi_stati_sub3", "wifi_stat_gate3", "wifi_usestadns3", "wifi_stat_dnsA3", "wifi_stat_dnsB3", "wifi_stat_dnsC3"},
        WiFiConfig{"wifi_ssid4", "wifi_key4", "wifi_usestatic4", "wifi_static_ip4", "wifi_stati_sub4", "wifi_stat_gate4", "wifi_usestadns4", "wifi_stat_dnsA4", "wifi_stat_dnsB4", "wifi_stat_dnsC4"},
        WiFiConfig{"wifi_ssid5", "wifi_key5", "wifi_usestatic5", "wifi_static_ip5", "wifi_stati_sub5", "wifi_stat_gate5", "wifi_usestadns5", "wifi_stat_dnsA5", "wifi_stat_dnsB5", "wifi_stat_dnsC5"},
        WiFiConfig{"wifi_ssid6", "wifi_key6", "wifi_usestatic6", "wifi_static_ip6", "wifi_stati_sub6", "wifi_stat_gate6", "wifi_usestadns6", "wifi_stat_dnsA6", "wifi_stat_dnsB6", "wifi_stat_dnsC6"},
        WiFiConfig{"wifi_ssid7", "wifi_key7", "wifi_usestatic7", "wifi_static_ip7", "wifi_stati_sub7", "wifi_stat_gate7", "wifi_usestadns7", "wifi_stat_dnsA7", "wifi_stat_dnsB7", "wifi_stat_dnsC7"},
        WiFiConfig{"wifi_ssid8", "wifi_key8", "wifi_usestatic8", "wifi_static_ip8", "wifi_stati_sub8", "wifi_stat_gate8", "wifi_usestadns8", "wifi_stat_dnsA8", "wifi_stat_dnsB8", "wifi_stat_dnsC8"},
        WiFiConfig{"wifi_ssid9", "wifi_key9", "wifi_usestatic9", "wifi_static_ip9", "wifi_stati_sub9", "wifi_stat_gate9", "wifi_usestadns9", "wifi_stat_dnsA9", "wifi_stat_dnsB9", "wifi_stat_dnsC9"},
    };

    struct : ConfigWrapper<bool>
    {
        bool allowReset() const final { return true; }
        const char* nvsName() const final { return "apEnabled"; }
        value_t defaultValue() const final { return true; }
        ConfigConstraintReturnType checkValue(value_t value) const final { return {}; }
    } wifiApEnabled;

    struct : ConfigWrapper<std::string>
    {
        bool allowReset() const final { return true; }
        const char *nvsName() const final { return "apName"; }
        std::string defaultValue() const final { return defaultHostname(); }
        ConfigConstraintReturnType checkValue(value_t value) const final { return StringMinMaxSize<4, 32>(value); }
    } wifiApName;

    struct : ConfigWrapper<std::string>
    {
        bool allowReset() const final { return true; }
        const char *nvsName() const final { return "wifiApKey"; }
        std::string defaultValue() const final { return "ws2812-clock"; }
        ConfigConstraintReturnType checkValue(value_t value) const final { return StringOr<StringEmpty, StringMinMaxSize<8, 64>>(value); }
    } wifiApKey;

    struct : ConfigWrapper<uint8_t>
    {
        bool allowReset() const final { return true; }
        const char *nvsName() const final { return "wifiApChannel"; }
        value_t defaultValue() const final { return 1; }
        ConfigConstraintReturnType checkValue(value_t value) const final { return MinMaxValue<uint8_t, 1, 14>(value); }
    } wifiApChannel;

    struct : ConfigWrapper<wifi_stack::ip_address_t>
    {
        bool allowReset() const final { return true; }
        const char *nvsName() const final { return "wifiApIp"; }
        value_t defaultValue() const final { return wifi_stack::ip_address_t{4, 3, 2, 1}; }
        ConfigConstraintReturnType checkValue(value_t value) const final { return {}; }
    } wifiApIp;

    struct : ConfigWrapper<wifi_stack::ip_address_t>
    {
        bool allowReset() const final { return true; }
        const char *nvsName() const final { return "wifiApMask"; }
        value_t defaultValue() const final { return wifi_stack::ip_address_t{255, 255, 255, 0}; }
        ConfigConstraintReturnType checkValue(value_t value) const final { return {}; }
    } wifiApMask;

    struct : ConfigWrapper<wifi_auth_mode_t>
    {
        bool allowReset() const final { return true; }
        const char *nvsName() const final { return "wifiApAuthmode"; }
        value_t defaultValue() const final { return WIFI_AUTH_WPA_WPA2_PSK; }
        ConfigConstraintReturnType checkValue(value_t value) const final { return {}; }
    } wifiApAuthmode;

    struct : ConfigWrapper<bool>
    {
        bool allowReset() const final { return true; }
        const char* nvsName() const final { return "apWhenNoWiFi"; }
        value_t defaultValue() const final { return false; }
        ConfigConstraintReturnType checkValue(value_t value) const final { return {}; }
    } wifiApOnlyWhenNotConnected;

    // LED
    struct : ConfigWrapper<uint8_t>
    {
        bool allowReset() const final { return true; }
        const char* nvsName() const final { return "ledBrightness"; }
        value_t defaultValue() const final { return 50; }
        ConfigConstraintReturnType checkValue(value_t value) const final { return {}; }
    } ledBrightness;
    struct : ConfigWrapper<uint8_t>
    {
        bool allowReset() const final { return true; }
        const char* nvsName() const final { return "secondaryBright"; }
        value_t defaultValue() const final { return 10; }
        ConfigConstraintReturnType checkValue(value_t value) const final { return {}; }
    } ledSecondaryBrightness;
    struct : ConfigWrapper<uint8_t>
    {
        bool allowReset() const final { return true; }
        const char* nvsName() const final { return "basicLedBright"; }
        value_t defaultValue() const final { return 10; }
        ConfigConstraintReturnType checkValue(value_t value) const final {
            if (value > 100) {
                return std::unexpected("Value must be between 0 and 100");
            }
            return {};
        }
    } basicLedBrightness;

    // Time
    struct : ConfigWrapper<minutes32>
    {
        bool allowReset() const final { return true; }
        const char *nvsName() const final { return "timeOffset"; }
        value_t defaultValue() const final { return 1h; }
        ConfigConstraintReturnType checkValue(value_t value) const final { return {}; }
    } timeOffset;
    struct : ConfigWrapper<DayLightSavingMode>
    {
        bool allowReset() const final { return true; }
        const char *nvsName() const final { return "daylightSaving"; }
        value_t defaultValue() const final { return DayLightSavingMode::EuropeanSummerTime; }
        ConfigConstraintReturnType checkValue(value_t value) const final { return {}; }
    } timeDst;
    TimeRangeConfigWrapper secondaryBrightnessTimeRange{"secoBriRaStart", "secoBriRaEnd"};
    struct : ConfigWrapper<SecondaryBrightnessMode>
    {
        bool allowReset() const final { return true; }
        const char *nvsName() const final { return "seconBrightMode"; }
        value_t defaultValue() const final { return SecondaryBrightnessMode::Off; }
        ConfigConstraintReturnType checkValue(value_t value) const final { return {}; }
    } secondaryBrightnessMode;

    // NTP
    struct : ConfigWrapper<std::string>
    {
        bool allowReset() const final { return false; }
        const char *nvsName() const final { return "timeServer"; }
        std::string defaultValue() const final { return "europe.pool.ntp.org"; }
        ConfigConstraintReturnType checkValue(value_t value) const final { return StringMaxSize<64>(value); }
    } timeServer;
    struct : ConfigWrapper<sntp_sync_mode_t>
    {
        bool allowReset() const final { return false; }
        const char *nvsName() const final { return "timeSyncMode"; }
        value_t defaultValue() const final { return SNTP_SYNC_MODE_IMMED; }
        ConfigConstraintReturnType checkValue(value_t value) const final { return {}; }
    } timeSyncMode;
    struct : ConfigWrapper<milliseconds32>
    {
        bool allowReset() const final { return false; }
        const char *nvsName() const final { return "tSyncInterval"; }
        value_t defaultValue() const final { return milliseconds32{CONFIG_LWIP_SNTP_UPDATE_DELAY}; }
        ConfigConstraintReturnType checkValue(value_t value) const final { return MinTimeSyncInterval(value); }
    } timeSyncInterval;

    // MQTT
    struct : ConfigWrapper<bool>
    {
        bool allowReset() const final { return true; }
        const char *nvsName() const final { return "mqttEnabled"; }
        value_t defaultValue() const final { return true; }
        ConfigConstraintReturnType checkValue(value_t value) const final { return {}; }
    } mqttEnabled;
    struct : ConfigWrapper<std::string>
    {
        bool allowReset() const final { return true; }
        const char *nvsName() const final { return "mqttUrl"; }
        std::string defaultValue() const final { return "mqtt://test.mosquitto.org:1883"; }
        ConfigConstraintReturnType checkValue(value_t value) const final { return StringMaxSize<64>(value); }
    } mqttUrl;
    struct : ConfigWrapper<std::string>
    {
        bool allowReset() const final { return true; }
        const char *nvsName() const final { return "mqttTopic"; }
        std::string defaultValue() const final { return "ws2812b-clock"; }
        ConfigConstraintReturnType checkValue(value_t value) const final { return StringMinMaxSize<1, 64>(value); }
    } mqttTopic;
    struct : ConfigWrapper<std::string>
    {
        bool allowReset() const final { return true; }
        const char *nvsName() const final { return "hassMqttTopic"; }
        std::string defaultValue() const final { return "homeassistant/"; }
        ConfigConstraintReturnType checkValue(value_t value) const final { return StringMinMaxSize<1, 64>(value); }
    } hassMqttTopic;
    struct : ConfigWrapper<milliseconds32>
    {
        bool allowReset() const final { return false; }
        const char *nvsName() const final { return "mqttPubInterval"; }
        value_t defaultValue() const final { return milliseconds32{60000}; } // 1 minute
        ConfigConstraintReturnType checkValue(value_t value) const final { return {}; }
    } mqttPublishInterval;

    // Customization
    /*-- Hide Clock while NTP sync has not finished --*/
    struct : ConfigWrapper<bool>
    {
        bool allowReset() const final { return true; }
        const char *nvsName() const final { return "showUnsyncdTime"; }
        value_t defaultValue() const final { return false; }
        ConfigConstraintReturnType checkValue(value_t value) const final { return {}; }
    } showUnsyncedTime;
    struct : ConfigWrapper<LedAnimationName>
    {
        bool allowReset() const final { return true; }
        const char *nvsName() const final { return "ledAnimation"; }
        value_t defaultValue() const final { return animation::getFirstAnimation().getEnumValue(); }
        ConfigConstraintReturnType checkValue(value_t value) const final
        {
            if (!animation::animationExists(value))
                return std::unexpected("Invalid animation name");
            return {};
        }
    } ledAnimation;
    struct : ConfigWrapper<bool>
    {
        bool allowReset() const final { return true; }
        const char *nvsName() const final { return "ledAnimationEna"; }
        value_t defaultValue() const final { return true; }
        ConfigConstraintReturnType checkValue(value_t value) const final { return {}; }
    } ledAnimationEnabled;
    struct : ConfigWrapper<ColorHelper>
    {
        bool allowReset() const final { return true; }
        const char *nvsName() const final { return "primaryColor"; }
        value_t defaultValue() const final { return { 255, 235, 235 }; }
        ConfigConstraintReturnType checkValue(value_t value) const final { return {}; }
    } primaryColor;
    struct : ConfigWrapper<ColorHelper>
    {
        bool allowReset() const final { return true; }
        const char *nvsName() const final { return "secondaryColor"; }
        value_t defaultValue() const final { return cpputils::white; }
        ConfigConstraintReturnType checkValue(value_t value) const final { return {}; }
    } secondaryColor;
    struct : ConfigWrapper<ColorHelper>
    {
        bool allowReset() const final { return true; }
        const char *nvsName() const final { return "tertiaryColor"; }
        value_t defaultValue() const final { return cpputils::black; }
        ConfigConstraintReturnType checkValue(value_t value) const final { return {}; }
    } tertiaryColor;
    struct : ConfigWrapper<float>
    {
        bool allowReset() const final { return true; }
        const char *nvsName() const final { return "animMultiplier"; }
        value_t defaultValue() const final { return 1; }
        ConfigConstraintReturnType checkValue(value_t value) const final { return {}; }
    } animationMultiplier;

    /*-- Disable dot blinking --*/
    struct : ConfigWrapper<bool>
    {
        bool allowReset() const final { return true; }
        const char *nvsName() const final { return "disableDotBlink"; }
        value_t defaultValue() const final { return true; }
        ConfigConstraintReturnType checkValue(value_t value) const final { return {}; }
    } disableDotBlinking;
    struct : ConfigWrapper<bool>
    {
        bool allowReset() const final { return true; }
        const char *nvsName() const final { return "noClockDigits"; }
        value_t defaultValue() const final { return false; }
        ConfigConstraintReturnType checkValue(value_t value) const final { return {}; }
    } noClockDigits;


    /*-- Beeper --*/
    struct : ConfigWrapper<uint8_t>
    {
        bool allowReset() const final { return true; }
        const char *nvsName() const final { return "beepVolume"; }
        value_t defaultValue() const final { return 0; }
        ConfigConstraintReturnType checkValue(value_t value) const final { return {}; }
    } beeperVolume;

    template<typename T>
    void callForEveryConfig(T&& callable)
    {
#define ITER_CONFIG(name) \
    if (callable(name)) return;

        // Connectivity
        ITER_CONFIG(hostname)
        ITER_CONFIG(name)
        ITER_CONFIG(baseMacAddressOverride)
        ITER_CONFIG(wifiStaEnabled)

        for (auto& wifi : wifis)
        {
            ITER_CONFIG(wifi.ssid)
            ITER_CONFIG(wifi.key)
            ITER_CONFIG(wifi.useStaticIp)
            ITER_CONFIG(wifi.staticIp)
            ITER_CONFIG(wifi.staticSubnet)
            ITER_CONFIG(wifi.staticGateway)
            ITER_CONFIG(wifi.useStaticDns)
            ITER_CONFIG(wifi.staticDns0)
            ITER_CONFIG(wifi.staticDns1)
            ITER_CONFIG(wifi.staticDns2)
        }

        ITER_CONFIG(wifiApEnabled)
        ITER_CONFIG(wifiApOnlyWhenNotConnected)
        ITER_CONFIG(wifiApName)
        ITER_CONFIG(wifiApKey)
        ITER_CONFIG(wifiApChannel)
        ITER_CONFIG(wifiApIp)
        ITER_CONFIG(wifiApMask)
        ITER_CONFIG(wifiApAuthmode)

        // LED
        ITER_CONFIG(ledBrightness)
        ITER_CONFIG(ledSecondaryBrightness)
        ITER_CONFIG(basicLedBrightness)

        // Time
        ITER_CONFIG(timeOffset)
        ITER_CONFIG(timeDst)
        ITER_CONFIG(secondaryBrightnessTimeRange.start)
        ITER_CONFIG(secondaryBrightnessTimeRange.end)
        ITER_CONFIG(secondaryBrightnessMode)

        // NTP
        ITER_CONFIG(timeServer)
        ITER_CONFIG(timeSyncMode)
        ITER_CONFIG(timeSyncInterval)

        // MQTT
        ITER_CONFIG(mqttEnabled)
        ITER_CONFIG(mqttUrl)
        ITER_CONFIG(mqttTopic)
        ITER_CONFIG(hassMqttTopic)
        ITER_CONFIG(mqttPublishInterval)

        // Customization
        ITER_CONFIG(showUnsyncedTime)
        ITER_CONFIG(ledAnimation)
        ITER_CONFIG(ledAnimationEnabled)
        ITER_CONFIG(primaryColor)
        ITER_CONFIG(secondaryColor)
        ITER_CONFIG(tertiaryColor)
        ITER_CONFIG(animationMultiplier)
        ITER_CONFIG(disableDotBlinking)
        ITER_CONFIG(noClockDigits)

        // Beeper
        ITER_CONFIG(beeperVolume)

#undef ITER_CONFIG
    }
};
