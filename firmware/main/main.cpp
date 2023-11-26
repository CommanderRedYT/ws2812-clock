constexpr const char * const TAG = "main";

// esp-idf includes
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

// esp-idf optional includes
#if defined(CONFIG_ESP_TASK_WDT_PANIC) || defined(CONFIG_ESP_TASK_WDT)
#include <esp_task_wdt.h>
#endif

// local includes
#include "utils/config.h"
#include "utils/global_lock.h"
#include "utils/tasks.h"

/*--- Config Flag Error Handling ---*/
#if defined(HARDWARE_USE_SHT30) && defined(HARDWARE_USE_BME280)

#error "BME280 and SHT30 are mutually exclusive."

#endif

#if !defined(HARDWARE_WS2812B_PIN) || !defined(HARDWARE_WS2812B_COUNT)

#error "WS2812B pin and count must be defined."

#endif

using namespace std::chrono_literals;

extern "C" [[noreturn]] void app_main()
{
    ESP_LOGI(TAG, "app_main()");

    /*--- Task Watchdog ---*/
#if defined(CONFIG_ESP_TASK_WDT_PANIC) || defined(CONFIG_ESP_TASK_WDT)
    {
        const auto taskHandle = xTaskGetCurrentTaskHandle();
        if (!taskHandle)
        {
            ESP_LOGE(TAG, "could not get handle to current main task!");
        }
        else if (const auto result = esp_task_wdt_add(taskHandle); result != ESP_OK)
        {
            ESP_LOGE(TAG, "could not add main task to watchdog: %s", esp_err_to_name(result));
        }
    }
#endif


    /*--- Settings ---*/
    if (const auto result = configs.init("ws2812-clock"); result != ESP_OK)
        ESP_LOGE(TAG, "config_init_settings() failed with %s", esp_err_to_name(result));
    else
        ESP_LOGI(TAG, "config_init_settings() succeeded");

    configs.callForEveryConfig([&](auto& config) {
        if (strlen(config.nvsName()) > 15)
        {
            ESP_LOGW(TAG, "config key '%s' is longer than 15 characters", config.nvsName());
            while (true)
                vTaskDelay(1000);
        }

        return false;
    });


    /*--- Global Lock ---*/
    global::global_lock.construct();
    ESP_LOGI(TAG, "global lock constructed");


    /*--- Task Manager ---*/
    for (const auto& task : tasks)
        task.setup();

#if defined(CONFIG_ESP_TASK_WDT_PANIC) || defined(CONFIG_ESP_TASK_WDT)
    if (const auto result = esp_task_wdt_reset(); result != ESP_OK)
        ESP_LOGE(TAG, "esp_task_wdt_reset() failed with %s", esp_err_to_name(result));
#endif

    espchrono::millis_clock::time_point lastTaskPush = espchrono::millis_clock::now();

    while (true)
    {
        for (auto& task : tasks)
        {
            task.loop();

#if defined(CONFIG_ESP_TASK_WDT_PANIC) || defined(CONFIG_ESP_TASK_WDT)
            if (const auto result = esp_task_wdt_reset(); result != ESP_OK)
                ESP_LOGE(TAG, "esp_task_wdt_reset() failed with %s", esp_err_to_name(result));
#endif
        }

        if (espchrono::ago(lastTaskPush) > 1s)
        {
            lastTaskPush = espchrono::millis_clock::now();
            sched_pushStats(false);
        }

        vTaskDelay(1);
    }
}
