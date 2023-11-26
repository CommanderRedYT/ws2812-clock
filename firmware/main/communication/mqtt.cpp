#include "mqtt.h"

constexpr const char * const TAG = "mqtt";

// system includes
#include <tuple>

// 3rdparty lib includes
#include <cleanuphelper.h>
#include <lockingqueue.h>
#include <recursivelockhelper.h>
#include <taskutils.h>
#include <wrappers/mqtt_client.h>

// local includes
#include "communication/helper/status.h"
#include "utils/config.h"
#include "utils/global_lock.h"
#include "communication/wifi.h"

namespace mqtt {

espcpputils::mqtt_client client;

namespace {

espcpputils::LockingQueue<std::tuple<std::string, std::string>> publishQueue;

enum class MqttState
{
    NotStarted = 0,
    Error,
    Initialized,
    Started,
    Connecting,
    Connected,
} mqttState;

std::string lastMqttUrl;
espchrono::millis_clock::time_point lastMqttPublish;
espchrono::millis_clock::time_point lastMqttHassPublish;

} // namespace

namespace {

std::string format_error(esp_mqtt_error_codes_t* error_handle)
{
    std::string error_type;
    std::string connect_return_code;

    switch(error_handle->error_type)
    {
    case MQTT_ERROR_TYPE_NONE:
        error_type = "MQTT_ERROR_TYPE_NONE";
        break;
    case MQTT_ERROR_TYPE_TCP_TRANSPORT:
        error_type = "MQTT_ERROR_TYPE_TCP_TRANSPORT";
        break;
    case MQTT_ERROR_TYPE_CONNECTION_REFUSED:
        error_type = "MQTT_ERROR_TYPE_CONNECTION_REFUSED";
        break;
    case MQTT_ERROR_TYPE_SUBSCRIBE_FAILED:
        error_type = "MQTT_ERROR_TYPE_SUBSCRIBE_FAILED";
        break;
    }

    switch(error_handle->connect_return_code)
    {
    case MQTT_CONNECTION_ACCEPTED:
        connect_return_code = "MQTT_CONNECTION_ACCEPTED";
        break;
    case MQTT_CONNECTION_REFUSE_PROTOCOL:
        connect_return_code = "MQTT_CONNECTION_REFUSE_PROTOCOL";
        break;
    case MQTT_CONNECTION_REFUSE_ID_REJECTED:
        connect_return_code = "MQTT_CONNECTION_REFUSE_ID_REJECTED";
        break;
    case MQTT_CONNECTION_REFUSE_SERVER_UNAVAILABLE:
        connect_return_code = "MQTT_CONNECTION_REFUSE_SERVER_UNAVAILABLE";
        break;
    case MQTT_CONNECTION_REFUSE_BAD_USERNAME:
        connect_return_code = "MQTT_CONNECTION_REFUSE_BAD_USERNAME";
        break;
    case MQTT_CONNECTION_REFUSE_NOT_AUTHORIZED:
        connect_return_code = "MQTT_CONNECTION_REFUSE_NOT_AUTHORIZED";
        break;
    }

    return fmt::format("error_type: {}, connect_return_code: {}", error_type, connect_return_code);
}

[[noreturn]] void mqtt_handle_send(void*)
{
    auto helper = cpputils::makeCleanupHelper([](){ vTaskDelete(nullptr); });

    while (true)
    {
        while (auto entry = publishQueue.tryPop())
        {
            espcpputils::RecursiveLockHelper guard{global::global_lock->handle};

            if (!client)
            {
                ESP_LOGE(TAG, "mqtt_send_handle: client not initialized");
                publishQueue.clear();
                continue;
            }

            if (client.publish(std::get<0>(*entry), std::get<1>(*entry), 0, 1) < 0)
            {
                ESP_LOGE(TAG, "mqtt_send_handle: publish failed");
                publishQueue.clear();
                continue;
            }
        }

        vTaskDelay(pdMS_TO_TICKS(100)); // delay 100ms
    }
}

void publishStatus()
{
    status::forEveryKey([&](const JsonString& key, const JsonVariant& value){
        // ESP_LOGI(TAG, "publishStatus: %s=%s", key.c_str(), value.as<char*>());
        std::string topic = fmt::format("{}/{}/status/{}", configs.mqttTopic.value(), configs.hostname.value(), key.c_str());
        std::string valueBuffer;

        serializeJson(value, valueBuffer);

        publishQueue.push(std::make_tuple(topic, valueBuffer));

        // ESP_LOGI(TAG, "publishStatus: %s=%s", topic.c_str(), valueBuffer.c_str());

        return ESP_OK;
    });

    lastMqttPublish = espchrono::millis_clock::now();
}

void publishHomeassistantDiscovery()
{
    // hard code for the bme
#ifdef HARDWARE_USE_BME280
    // {mqttTopic}/{hostname}/status/bme280/temp: 25 (°C)
    // {mqttTopic}/{hostname}/status/bme280/pressure (hPa)
    // {mqttTopic}/{hostname}/status/bme280/humidity (%)
    // {mqttTopic}/{hostname}/status/bme280/altitude (m)
    std::string payload;

    constexpr const char * const FORMAT_STR = R"({{
        "name": "{}{}",
        "device_class": "{}",
        "state_topic": "{}/{}/status/bme280/{}",
        "unit_of_measurement": "°C",
        "value_template": "{{{{ value_json.{} }}}}"
    }})";

    payload = fmt::format(
            FORMAT_STR,
            "Temperature",
            configs.hostname.value(),
            "temperature",
            configs.mqttTopic.value(),
            configs.hostname.value(),
            "temp",
            "temp"
        );
    publishQueue.push(std::make_tuple(fmt::format("{}sensor/{}/temp/config", configs.hassMqttTopic.value(), configs.hostname.value()), payload));

    payload = fmt::format(
            FORMAT_STR,
            "Pressure",
            configs.hostname.value(),
            "pressure",
            configs.mqttTopic.value(),
            configs.hostname.value(),
            "pressure",
            "pressure"
        );

    publishQueue.push(std::make_tuple(fmt::format("{}sensor/{}/pressure/config", configs.hassMqttTopic.value(), configs.hostname.value()), payload));

    payload = fmt::format(
            FORMAT_STR,
            "Humidity",
            configs.hostname.value(),
            "humidity",
            configs.mqttTopic.value(),
            configs.hostname.value(),
            "humidity",
            "humidity"
        );

    publishQueue.push(std::make_tuple(fmt::format("{}sensor/{}/humidity/config", configs.hassMqttTopic.value(), configs.hostname.value()), payload));

    payload = fmt::format(
            FORMAT_STR,
            "Altitude",
            configs.hostname.value(),
            "altitude",
            configs.mqttTopic.value(),
            configs.hostname.value(),
            "altitude",
            "altitude"
        );

    publishQueue.push(std::make_tuple(fmt::format("{}sensor/{}/altitude/config", configs.hassMqttTopic.value(), configs.hostname.value()), payload));
#endif

    lastMqttHassPublish = espchrono::millis_clock::now();
}

void event_handler(void*, esp_event_base_t event_base, int32_t event_id_arg, void *event_data)
{
    auto* data = reinterpret_cast<const esp_mqtt_event_t*>(event_data);
    auto event_id = static_cast<esp_mqtt_event_id_t>(event_id_arg);

    switch (event_id)
    {
    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAG, "%s event_id=MQTT_EVENT_ERROR (%d)", event_base, event_id);

        if (data->error_handle)
        {
            ESP_LOGE(TAG, "MQTT_EVENT_ERROR: %s", format_error(data->error_handle).c_str());
        }
        else
        {
            ESP_LOGE(TAG, "MQTT_EVENT_ERROR: no error_handle");
        }
        mqttState = MqttState::Error;
        break;
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "%s event_id=MQTT_EVENT_CONNECTED (%d)", event_base, event_id);

        mqttState = MqttState::Connected;

        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "%s event_id=MQTT_EVENT_DISCONNECTED (%d)", event_base, event_id);

        mqttState = MqttState::Initialized;

        break;
    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG, "%s event_id=MQTT_EVENT_SUBSCRIBED (%d)", event_base, event_id);
        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(TAG, "%s event_id=MQTT_EVENT_UNSUBSCRIBED (%d)", event_base, event_id);
        break;
    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG, "%s event_id=MQTT_EVENT_PUBLISHED (%d)", event_base, event_id);

        break;
    case MQTT_EVENT_DATA:
    {
        std::string_view topic{data->topic, static_cast<size_t>(data->topic_len)};
        std::string_view payload{data->data, static_cast<size_t>(data->data_len)};

        ESP_LOGI(TAG, "%s event_id=MQTT_EVENT_DATA (%d), topic=%.*s, payload=%.*s",
                 event_base,
                 event_id,
                 topic.size(),
                 topic.data(),
                 payload.size(),
                 payload.data());
        break;
    }
    case MQTT_EVENT_BEFORE_CONNECT:
        ESP_LOGI(TAG, "%s event_id: %d", event_base, event_id);
        break;
    case MQTT_EVENT_DELETED:
        ESP_LOGI(TAG, "%s event_id: %d", event_base, event_id);
        break;
    default:;
    }
}

void init(std::string_view url)
{
    espcpputils::RecursiveLockHelper guard{global::global_lock->handle};

    client = {};
    lastMqttUrl = {};
    mqttState = MqttState::NotStarted;

    const auto result = espcpputils::createTask(mqtt_handle_send, "mqttSend", 4096, nullptr, 5, nullptr, espcpputils::CoreAffinity::Both);
    if (result != pdPASS)
    {
        auto msg = fmt::format("failed creating mqtt task {}", result);
        ESP_LOGE(TAG, "%.*s", msg.size(), msg.data());
        return;
    }

    esp_mqtt_client_config_t mqtt_cfg{
        .broker = {
            .address = {
                .uri = url.data(),
            },
        },
    };

    client = espcpputils::mqtt_client{&mqtt_cfg};
    if (!client)
    {
        ESP_LOGE(TAG, "Failed to initialize MQTT client");
        mqttState = MqttState::Error;

        return;
    }

    lastMqttUrl = url;

    if (const auto res = client.register_event(MQTT_EVENT_ANY, event_handler, nullptr); res != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to register MQTT event handler: %s", esp_err_to_name(res));
        mqttState = MqttState::Error;

        return;
    }

    mqttState = MqttState::Initialized;
}

void handle_start()
{
    if (!client)
    {
        ESP_LOGE(TAG, "MQTT client is not initialized");
        return;
    }

    if (const auto res = client.start(); res != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to start MQTT client: %s", esp_err_to_name(res));
        mqttState = MqttState::Error;
    }
    else
    {
        ESP_LOGI(TAG, "MQTT client started");
        mqttState = MqttState::Connecting;
    }
}

void handle_stop()
{
    if (mqttState < MqttState::Started)
        return;

    if (!client)
    {
        ESP_LOGD(TAG, "MQTT client is not initialized");
        return;
    }

    if (const auto res = client.stop(); res != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to stop MQTT client: %s (mqttState=%d)", esp_err_to_name(res), std::to_underlying(mqttState));
        mqttState = MqttState::Error;
    }
    else
    {
        ESP_LOGI(TAG, "MQTT client stopped");
        mqttState = MqttState::Initialized;
    }
}

} // namespace

void begin()
{
    if (!configs.mqttEnabled.value())
        return;

    if (const auto value = configs.mqttUrl.value(); value.empty())
    {
        return;
    }
    else
    {
        init(value);
    }
}

void update()
{
    espcpputils::RecursiveLockHelper guard{global::global_lock->handle};

    if ((!configs.mqttEnabled.value() && mqttState != MqttState::NotStarted) || !wifi::isStaConnected())
    {
        handle_stop();

        client = {};
        lastMqttUrl = {};

        if (mqttState != MqttState::Error)
            mqttState = MqttState::NotStarted;

        return;
    }

    if (configs.mqttEnabled.value() && mqttState == MqttState::NotStarted)
    {
        if (const auto value = configs.mqttUrl.value(); value.empty())
        {
            return;
        }
        else
        {
            init(value);
        }
    }

    if (mqttState != MqttState::NotStarted)
    {
        if (const auto value = configs.mqttUrl.value(); !value.empty() && value != lastMqttUrl)
        {
            init(value);
        }

        if (mqttState == MqttState::Initialized)
        {
            handle_start();
        }
    }

    if (mqttState == MqttState::Connected && espchrono::ago(lastMqttPublish) > 1s)
    {
        publishStatus();
    }

    if (mqttState == MqttState::Connected && espchrono::ago(lastMqttHassPublish) > 30s)
    {
        publishHomeassistantDiscovery();
    }
}

} // namespace mqtt
