#include "basicleds.h"

constexpr const char * const TAG = "BASICLEDS";

// esp-idf includes
#include <driver/ledc.h>

// 3rdparty lib includes
#include <espchrono.h>

// local includes
#include "communication/wifi.h"
#include "utils/config.h"

#define RESOLUTION LEDC_TIMER_13_BIT
#define MAX_DUTY (1 << RESOLUTION)

namespace basicleds {

LedConfig ledConfig;

namespace {

void handleLed(Led& led, ledc_channel_t channel)
{
    const auto now = espchrono::millis_clock::now();

    if (!led.lastUpdate)
    {
        led.lastUpdate = now;
    }

    const auto delta = now - *led.lastUpdate;

    if (led.current == led.target)
    {
        return;
    }

    if (led.current < led.target)
    {
        led.current += delta.count() * 1000 / 1000;
        if (led.current > led.target)
        {
            led.current = led.target;
        }
    }
    else
    {
        led.current -= delta.count() * 1000 / 1000;
        if (led.current < led.target)
        {
            led.current = led.target;
        }
    }

    led.lastUpdate = now;

    const auto configuredBrightness = configs.basicLedBrightness.value(); // 0-100
    const auto actualMaxDuty = MAX_DUTY * configuredBrightness / 100;
    const auto duty = led.current * actualMaxDuty / 100;

    ESP_LOGI(TAG, "led.current: %d, actualMaxDuty: %d, duty: %d", led.current, actualMaxDuty, duty);

    ledc_set_duty(LEDC_HIGH_SPEED_MODE, channel, duty);
    ledc_update_duty(LEDC_HIGH_SPEED_MODE, channel);
}
} // namespace

void begin()
{
    // PIN: HARDWARE_AP_STATUS_LED_PIN
    // PIN: HARDWARE_STA_STATUS_LED_PIN
    // PIN: HARDWARE_ALARM_STATUS_LED_PIN

    // initialize the LED GPIOs and turn them on
    ledc_timer_config_t ledc_timer = {
        .speed_mode = LEDC_HIGH_SPEED_MODE,   // timer mode
        .duty_resolution = RESOLUTION,        // resolution of PWM duty
        .timer_num = LEDC_TIMER_0,            // timer index
        .freq_hz = 5000,                      // frequency of PWM signal
        .clk_cfg = LEDC_AUTO_CLK,             // Auto select the source clock
    };

    ledc_timer_config(&ledc_timer);

    ledc_channel_config_t ledc_channel_ap = {
        .gpio_num = HARDWARE_AP_STATUS_LED_PIN,
        .speed_mode = LEDC_HIGH_SPEED_MODE,
        .channel = LEDC_CHANNEL_0,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = LEDC_TIMER_0,
        .duty = 0,
        .hpoint = 0,
    };

    ledc_channel_config_t ledc_channel_sta = {
        .gpio_num = HARDWARE_STA_STATUS_LED_PIN,
        .speed_mode = LEDC_HIGH_SPEED_MODE,
        .channel = LEDC_CHANNEL_1,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = LEDC_TIMER_0,
        .duty = 0,
        .hpoint = 0,
    };

    ledc_channel_config_t ledc_channel_alarm = {
        .gpio_num = HARDWARE_ALARM_STATUS_LED_PIN,
        .speed_mode = LEDC_HIGH_SPEED_MODE,
        .channel = LEDC_CHANNEL_2,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = LEDC_TIMER_0,
        .duty = 0,
        .hpoint = 0,
    };

    ledc_channel_config(&ledc_channel_ap);
    ledc_channel_config(&ledc_channel_sta);
    ledc_channel_config(&ledc_channel_alarm);

    ledc_fade_func_install(0);

    ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0, 0);
    ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_1, 0);
    ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_2, 0);

    ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0);
    ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_1);
    ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_2);

    ledConfig.ap_status_led.current = 0;
    ledConfig.ap_status_led.target = 0;
    ledConfig.wifi_status_led.current = 0;
    ledConfig.wifi_status_led.target = 0;
    ledConfig.alarm_status_led.current = 0;
    ledConfig.alarm_status_led.target = 0;
}

void update()
{
    // const auto now = espchrono::millis_clock::now();

    if (wifi::isStaConnected())
    {
        ledConfig.wifi_status_led.target = 100;
    }
    else
    {
        ledConfig.wifi_status_led.target = 0;
    }

    if (wifi::isApUp())
    {
        ledConfig.ap_status_led.target = 100;
    }
    else
    {
        ledConfig.ap_status_led.target = 0;
    }

    handleLed(ledConfig.wifi_status_led, LEDC_CHANNEL_0);
    handleLed(ledConfig.ap_status_led, LEDC_CHANNEL_1);
    handleLed(ledConfig.alarm_status_led, LEDC_CHANNEL_2);
}

} // namespace basicleds
