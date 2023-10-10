#include "bme280.h"
#ifdef HARDWARE_USE_BME280

constexpr const char * const TAG = "sensors::bme280";

// system includes
#include <expected>
#include <string>

// esp-idf includes
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

// 3rdparty lib includes
#include <bmx280.h>
#include <cleanuphelper.h>
#include <espchrono.h>
#include <fmt/format.h>

namespace bme280_sensor {

BME280 bme280;

TaskHandle_t update_task_handle{nullptr};

void bmx280_task(void*)
{
    auto helper = cpputils::makeCleanupHelper([](){ vTaskDelete(nullptr); });

    i2c_config_t i2c_cfg = {
            .mode = I2C_MODE_MASTER,
            .sda_io_num = HARDWARE_I2C_SDA,
            .scl_io_num = HARDWARE_I2C_SCL,
            .sda_pullup_en = true,
            .scl_pullup_en = true,
            .master = {
                .clk_speed = 100000
            }
    };

    if (auto res = i2c_param_config(I2C_NUM_0, &i2c_cfg); res != ESP_OK)
    {
        ESP_LOGE(TAG, "i2c_param_config failed: %s", esp_err_to_name(res));
        return;
    }
    else
        ESP_LOGI(TAG, "i2c_param_config ok");

    if (auto res = i2c_driver_install(I2C_NUM_0, I2C_MODE_MASTER, 0, 0, 0); res != ESP_OK)
    {
        ESP_LOGE(TAG, "i2c_driver_install failed: %s", esp_err_to_name(res));
        return;
    }
    else
        ESP_LOGI(TAG, "i2c_driver_install ok");

    bme280.dev = bmx280_create(I2C_NUM_0);

    if (!bme280.dev)
    {
        ESP_LOGE(TAG, "bmx280_create failed");
        return;
    }
    else
        ESP_LOGI(TAG, "bmx280_create ok");

    if (auto res = bmx280_init(bme280.dev); res != ESP_OK)
    {
        ESP_LOGE(TAG, "bmx280_init failed: %s", esp_err_to_name(res));
        return;
    }
    else
        ESP_LOGI(TAG, "bmx280_init ok");

    bmx280_config_t bmx_cfg = BMX280_DEFAULT_CONFIG;

    if (auto res = bmx280_configure(bme280.dev, &bmx_cfg); res != ESP_OK)
    {
        ESP_LOGE(TAG, "bmx280_set_config failed: %s", esp_err_to_name(res));
        return;
    }
    else
        ESP_LOGI(TAG, "bmx280_set_config ok");

    bme280.initialized = true;

    while(true)
    {
        if (!bme280.initialized)
            return;

        bme280.mutex.lock();

        if (auto res = bmx280_setMode(bme280.dev, BMX280_MODE_FORCE); res != ESP_OK)
        {
            ESP_LOGE(TAG, "bmx280_force failed: %s", esp_err_to_name(res));
            return;
        }

        do
        {
            vTaskDelay(pdMS_TO_TICKS(1));
        }
        while (bmx280_isSampling(bme280.dev));

        float temp, press, hum, alt = 0.0f;

        if (auto res = bmx280_readoutFloat(bme280.dev, &temp, &press, &hum, &alt); res != ESP_OK)
        {
            ESP_LOGE(TAG, "bmx280_readFloat failed: %s", esp_err_to_name(res));
            return;
        }

        if (!bme280.data)
            bme280.data.emplace();

        bme280.data->temperature = temp;
        bme280.data->pressure = press;
        bme280.data->humidity = hum;
        bme280.data->altitude = alt;
        bme280.data->timestamp = espchrono::millis_clock::now();

        bme280.mutex.unlock();

        ESP_LOGD(TAG, "%s", bme280.toString().c_str());

        vTaskDelay(1);
    }
}

void begin()
{
    xTaskCreate(bmx280_task, "bme280_update_task", 4096, nullptr, 5, &update_task_handle);
}

std::string BME280::toString() const
{
    return fmt::format("temperature: {:.2f}°C, pressure: {:.2f}hPa, humidity: {:.2f}%, altitude: {:.2f}m",
                       data->temperature, data->pressure, data->humidity, data->altitude);
}

std::expected<BME280SensorData, std::string> getData()
{
    if (!bme280.initialized)
        return {};

    bme280.mutex.lock();
    auto data = bme280.data;
    bme280.mutex.unlock();

    if (!data)
        return std::unexpected("no bme280 data available");

    return *data;
}
} // namespace sensors

#endif // HARDWARE_USE_BME280
