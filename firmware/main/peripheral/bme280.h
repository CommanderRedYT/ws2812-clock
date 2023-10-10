#pragma once
#ifdef HARDWARE_USE_BME280

// system includes
#include <atomic>
#include <expected>
#include <optional>
#include <string>

// 3rdparty lib includes
#include <bmx280.h>
#include <espchrono.h>

namespace bme280_sensor {

struct BME280SensorData
{
    float temperature{0.0f};
    float pressure{0.0f};
    float humidity{0.0f};
    float altitude{0.0f};
    espchrono::millis_clock::time_point timestamp;
};

struct BME280
{
    bool initialized{false};
    bmx280_t* dev{nullptr};

    std::optional<BME280SensorData> data{std::nullopt};

    std::string toString() const;

    std::mutex mutex;
};

extern BME280 bme280;

extern TaskHandle_t update_task_handle;

void bmx280_task(void*);

void begin();

std::expected<BME280SensorData, std::string> getData();

} // namespace sensors

#endif // HARDWARE_USE_BME280
