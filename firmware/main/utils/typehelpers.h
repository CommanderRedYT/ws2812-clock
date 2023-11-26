#pragma once

// system includes
#include <type_traits>
#include <chrono>
#include <optional>

namespace typeutils {

using namespace espconfig;

template<class T>
struct is_duration : std::false_type {};

template<class Rep, class Period>
struct is_duration<std::chrono::duration<Rep, Period>> : std::true_type {};

template <typename _Tp>
inline constexpr bool is_duration_v = is_duration<_Tp>::value;

template<typename T>
struct t_to_str
{
    t_to_str() = delete;
    ~t_to_str() = delete;

    static const char *str;
};

template<typename T>
const char *t_to_str<T>::str = "unknown";

#define DEFINE_FOR_TYPE2(TYPE, STR) \
    template<> inline const char *t_to_str<TYPE>::str = #STR; \
    template<> inline const char *t_to_str<TYPE const&>::str = #STR; \
    template<> inline const char *t_to_str<std::optional<TYPE>>::str = "std::optional<" #STR ">"; \
    template<> inline const char *t_to_str<std::optional<TYPE> const&>::str = "std::optional<" #STR ">"; \
    template<> inline const char *t_to_str<ConfigWrapper<TYPE>>::str = #STR; \
    template<> inline const char *t_to_str<ConfigWrapper<TYPE> const&>::str = #STR;

#define TYPE_VALUES(x) \
    x(bool) \
    x(int8_t) \
    x(uint8_t) \
    x(int16_t) \
    x(uint16_t) \
    x(int32_t) \
    x(uint32_t) \
    x(int64_t) \
    x(uint64_t) \
    x(float) \
    x(double) \
    x(std::string) \
    x(sntp_sync_mode_t) \
    x(espchrono::DayLightSavingMode) \
    x(wifi_auth_mode_t) \
    x(wifi_stack::mac_t) \
    x(wifi_stack::ip_address_t) \
    x(espchrono::milliseconds32) \
    x(espchrono::minutes32) \
    x(espchrono::seconds32) \
    x(SecondaryBrightnessMode) \
    x(LedAnimationName) \
    x(cpputils::ColorHelper)

#define DEFINE_FOR_TYPE(TYPE) DEFINE_FOR_TYPE2(TYPE, TYPE)
TYPE_VALUES(DEFINE_FOR_TYPE)
#undef DEFINE_FOR_TYPE

// type traits is_numeric_v for std::optional<T>
template<typename T>
struct is_numeric : std::is_arithmetic<T> {};

template<typename T>
struct is_numeric<std::optional<T>> : is_numeric<T> {};

template<typename T>
inline constexpr bool is_numeric_v = is_numeric<T>::value;

template<typename T>
struct is_optional : std::false_type {};

template<typename T>
struct is_optional<std::optional<T>> : std::true_type {};

template<typename T>
inline constexpr bool is_optional_v = is_optional<T>::value;

template<typename T>
struct optional_value_type {
    using type = T;
};

template<typename T>
struct optional_value_type<std::optional<T>> {
    using type = T;
};

template<typename T>
using optional_value_type_t = typename optional_value_type<T>::type;

} // namespace typeutils
