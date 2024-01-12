#include "mqtt.h"

constexpr const char * const TAG = "mqtt";

// system includes
#include <tuple>

// 3rdparty lib includes
#include <cleanuphelper.h>
#include <lockingqueue.h>
#include <numberparsing.h>
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
espcpputils::LockingQueue<std::tuple<std::string, std::string>> receiveQueue;

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
std::optional<espchrono::millis_clock::time_point> lastMqttPublish;
bool mqttHassPublished;

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

[[noreturn]] void mqtt_handle_receive(void*)
{
    static StaticJsonDocument<1024> doc;
    auto helper = cpputils::makeCleanupHelper([](){ vTaskDelete(nullptr); });

    while (true)
    {
        while (auto entry = receiveQueue.tryPop())
        {
            espcpputils::RecursiveLockHelper guard{global::global_lock->handle};
            espcpputils::RecursiveLockHelper ledGuard{ledmanager::led_lock->handle};

            if (!client)
            {
                ESP_LOGE(TAG, "mqtt_receive_handle: client not initialized");
                publishQueue.clear();
                continue;
            }

            ESP_LOGI(TAG, "mqtt_receive_handle: received message on topic %s: %s", std::get<0>(*entry).c_str(), std::get<1>(*entry).c_str());

            // {mqttTopic}/{hostname}/set/brightness
            // {mqttTopic}/{hostname}/set/animation

            if (std::get<0>(*entry).find(fmt::format("{}/{}/set/", configs.mqttTopic.value(), configs.hostname.value())) == 0)
            {
                const std::string topic = std::get<0>(*entry);
                const std::string value = std::get<1>(*entry);

                const std::string key = topic.substr(topic.find_last_of('/') + 1);

                if (key == "light")
                {
                    doc.clear();

                    if (deserializeJson(doc, value) != DeserializationError::Ok)
                    {
                        ESP_LOGE(TAG, "mqtt_receive_handle: failed to deserialize json");
                        continue;
                    }

                    // {"state": "ON", "brightness": 255, "effect": "rainbow"}
                    if (doc.containsKey("effect"))
                    {
                        const auto effect = doc["effect"].as<std::string>();
                        if (const auto parsed = parseLedAnimationName(effect); parsed)
                        {
                            configs.write_config(configs.ledAnimation, *parsed);
                        }
                        else
                        {
                            ESP_LOGE(TAG, "mqtt_receive_handle: unknown effect %s", effect.c_str());
                        }
                    }

                    if (doc.containsKey("state"))
                    {
                        const auto state = doc["state"].as<std::string>();
                        configs.write_config(configs.ledAnimationEnabled, state == "ON");
                    }

                    if (doc.containsKey("brightness"))
                    {
                        if (const auto brightness = doc["brightness"].as<int>(); brightness >= 0 && brightness <= 255)
                        {
                            configs.write_config(configs.ledBrightness, brightness);
                        }
                        else
                        {
                            ESP_LOGE(TAG, "mqtt_receive_handle: brightness out of range");
                        }
                    }

                    if (doc.containsKey("color"))
                    {
                        auto color = doc["color"].as<JsonObject>();
                        if (color.containsKey("r") && color.containsKey("g") && color.containsKey("b"))
                        {
                            const auto r = color["r"].as<uint8_t>();
                            const auto g = color["g"].as<uint8_t>();
                            const auto b = color["b"].as<uint8_t>();

                            configs.write_config(configs.primaryColor, cpputils::ColorHelper{r, g, b});
                        }
                    }

                    lastMqttPublish = std::nullopt;
                }
                else
                {
                    ESP_LOGE(TAG, "mqtt_receive_handle: unknown key %s", key.c_str());
                }
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
    static StaticJsonDocument<1024> doc;

    const std::optional<std::string> configurationUrl = []() -> std::optional<std::string> {
        if (const auto ip_result = wifi_stack::get_ip_info(wifi_stack::esp_netifs[ESP_IF_WIFI_STA]))
        {
            return fmt::format("http://{}/", wifi_stack::toString(ip_result->ip));
        }

        return std::nullopt;
    }();

    const long long expireAfter = configs.mqttPublishInterval.value() / 1s + 5;

    auto fillCommonStuff = [&](JsonDocument& doc) {
        doc["device"]["identifiers"][0] = configs.hostname.value();
        doc["device"]["name"] = configs.customName.value();
        doc["device"]["manufacturer"] = "ws2812-clock";
        doc["device"]["model"] = "ws2812-clock-BME280";
        doc["device"]["sw_version"] = VERSION;
        if (configurationUrl)
        {
            doc["device"]["configuration_url"] = *configurationUrl;
        }
        doc["expire_after"] = expireAfter;
    };

#ifdef HARDWARE_USE_BME280
    // {mqttTopic}/{hostname}/status/bme280/temp: 25 (°C)
    // {mqttTopic}/{hostname}/status/bme280/pressure (hPa)
    // {mqttTopic}/{hostname}/status/bme280/humidity (%)
    {
        doc.clear();
        doc["name"] = "BME280 Temperature";
        doc["state_topic"] = fmt::format("{}/{}/status/bme280/temp", configs.mqttTopic.value(), configs.hostname.value());
        doc["availability_topic"] = fmt::format("{}/{}/online", configs.mqttTopic.value(), configs.hostname.value());
        doc["payload_available"] = "true";
        doc["payload_not_available"] = "false";
        doc["unit_of_measurement"] = "°C";
        doc["value_template"] = "{{ value_json }}";
        doc["state_class"] = "measurement";
        doc["device_class"] = "temperature";
        doc["unique_id"] = fmt::format("{}_bme280_temp", configs.hostname.value());
        fillCommonStuff(doc);

        std::string payload;
        serializeJson(doc, payload);

        publishQueue.push(std::make_tuple(
                fmt::format("{}sensor/{}/temp/config", configs.hassMqttTopic.value(), configs.hostname.value()),
                payload));
    }

    {
        doc.clear();
        doc["name"] = "BME280 Pressure";
        doc["state_topic"] = fmt::format("{}/{}/status/bme280/pressure", configs.mqttTopic.value(), configs.hostname.value());
        doc["availability_topic"] = fmt::format("{}/{}/online", configs.mqttTopic.value(), configs.hostname.value());
        doc["payload_available"] = "true";
        doc["payload_not_available"] = "false";
        doc["unit_of_measurement"] = "Pa";
        doc["value_template"] = "{{ value_json }}";
        doc["state_class"] = "measurement";
        doc["device_class"] = "atmospheric_pressure";
        doc["unique_id"] = fmt::format("{}_bme280_pressure", configs.hostname.value());
        fillCommonStuff(doc);

        std::string payload;
        serializeJson(doc, payload);

        publishQueue.push(std::make_tuple(
                fmt::format("{}sensor/{}/pressure/config", configs.hassMqttTopic.value(), configs.hostname.value()),
                payload));
    }

    {
        doc.clear();
        doc["name"] = "BME280 Humidity";
        doc["state_topic"] = fmt::format("{}/{}/status/bme280/humidity", configs.mqttTopic.value(), configs.hostname.value());
        doc["availability_topic"] = fmt::format("{}/{}/online", configs.mqttTopic.value(), configs.hostname.value());
        doc["payload_available"] = "true";
        doc["payload_not_available"] = "false";
        doc["unit_of_measurement"] = "%";
        doc["value_template"] = "{{ value_json }}";
        doc["state_class"] = "measurement";
        doc["device_class"] = "humidity";
        doc["unique_id"] = fmt::format("{}_bme280_humidity", configs.hostname.value());
        fillCommonStuff(doc);

        std::string payload;
        serializeJson(doc, payload);

        publishQueue.push(std::make_tuple(
                fmt::format("{}sensor/{}/humidity/config", configs.hassMqttTopic.value(), configs.hostname.value()),
                payload));
    }
#endif

    // {mqttTopic}/{hostname}/status/sta/ssid (dBm)
    // {mqttTopic}/{hostname}/status/sta/bssid (dBm)
    // {mqttTopic}/{hostname}/status/sta/ip (dBm)
    // {mqttTopic}/{hostname}/status/sta/rssi (dBm)

    {
        doc.clear();
        doc["name"] = "WiFi Signal Strength";
        doc["state_topic"] = fmt::format("{}/{}/status/sta/rssi", configs.mqttTopic.value(), configs.hostname.value());
        doc["availability_topic"] = fmt::format("{}/{}/online", configs.mqttTopic.value(), configs.hostname.value());
        doc["payload_available"] = "true";
        doc["payload_not_available"] = "false";
        doc["unit_of_measurement"] = "dBm";
        doc["value_template"] = "{{ value_json }}";
        doc["state_class"] = "measurement";
        doc["device_class"] = "signal_strength";
        doc["unique_id"] = fmt::format("{}_wifi_rssi", configs.hostname.value());
        fillCommonStuff(doc);

        std::string payload;
        serializeJson(doc, payload);

        publishQueue.push(std::make_tuple(
                fmt::format("{}sensor/{}/rssi/config", configs.hassMqttTopic.value(), configs.hostname.value()),
                payload));
    }

    {
        doc.clear();
        doc["name"] = "WiFi SSID";
        doc["state_topic"] = fmt::format("{}/{}/status/sta/ssid", configs.mqttTopic.value(), configs.hostname.value());
        doc["availability_topic"] = fmt::format("{}/{}/online", configs.mqttTopic.value(), configs.hostname.value());
        doc["payload_available"] = "true";
        doc["payload_not_available"] = "false";
        doc["value_template"] = "{{ value_json }}";
        doc["unique_id"] = fmt::format("{}_wifi_ssid", configs.hostname.value());
        fillCommonStuff(doc);

        std::string payload;
        serializeJson(doc, payload);

        publishQueue.push(std::make_tuple(
                fmt::format("{}sensor/{}/ssid/config", configs.hassMqttTopic.value(), configs.hostname.value()),
                payload));
    }

    {
        doc.clear();
        doc["name"] = "WiFi BSSID";
        doc["state_topic"] = fmt::format("{}/{}/status/sta/bssid", configs.mqttTopic.value(), configs.hostname.value());
        doc["availability_topic"] = fmt::format("{}/{}/online", configs.mqttTopic.value(), configs.hostname.value());
        doc["payload_available"] = "true";
        doc["payload_not_available"] = "false";
        doc["value_template"] = "{{ value_json }}";
        doc["unique_id"] = fmt::format("{}_wifi_bssid", configs.hostname.value());
        fillCommonStuff(doc);

        std::string payload;
        serializeJson(doc, payload);

        publishQueue.push(std::make_tuple(
                fmt::format("{}sensor/{}/bssid/config", configs.hassMqttTopic.value(), configs.hostname.value()),
                payload));
    }

    {
        doc.clear();
        doc["name"] = "WiFi IP";
        doc["state_topic"] = fmt::format("{}/{}/status/sta/ip", configs.mqttTopic.value(), configs.hostname.value());
        doc["availability_topic"] = fmt::format("{}/{}/online", configs.mqttTopic.value(), configs.hostname.value());
        doc["payload_available"] = "true";
        doc["payload_not_available"] = "false";
        doc["value_template"] = "{{ value_json }}";
        doc["unique_id"] = fmt::format("{}_wifi_ip", configs.hostname.value());
        fillCommonStuff(doc);

        std::string payload;
        serializeJson(doc, payload);

        publishQueue.push(std::make_tuple(
                fmt::format("{}sensor/{}/ip/config", configs.hassMqttTopic.value(), configs.hostname.value()),
                payload));
    }

    // {mqttTopic}/{hostname}/status/time/millis (milliseconds since boot)

    {
        doc.clear();
        doc["name"] = "Uptime";
        doc["state_topic"] = fmt::format("{}/{}/status/time/millis", configs.mqttTopic.value(), configs.hostname.value());
        doc["availability_topic"] = fmt::format("{}/{}/online", configs.mqttTopic.value(), configs.hostname.value());
        doc["payload_available"] = "true";
        doc["payload_not_available"] = "false";
        doc["unit_of_measurement"] = "ms";
        doc["value_template"] = "{{ value_json }}";
        doc["state_class"] = "measurement";
        doc["device_class"] = "timestamp";
        doc["unique_id"] = fmt::format("{}_uptime", configs.hostname.value());
        fillCommonStuff(doc);

        std::string payload;
        serializeJson(doc, payload);

        publishQueue.push(std::make_tuple(
                fmt::format("{}sensor/{}/uptime/config", configs.hassMqttTopic.value(), configs.hostname.value()),
                payload));
    }

    {
        doc.clear();
        doc["name"] = "Light";
        doc["command_topic"] = fmt::format("{}/{}/set/light", configs.mqttTopic.value(), configs.hostname.value());
        doc["state_topic"] = fmt::format("{}/{}/status/led/homeassistant", configs.mqttTopic.value(), configs.hostname.value());
        doc["availability_topic"] = fmt::format("{}/{}/online", configs.mqttTopic.value(), configs.hostname.value());
        doc["payload_available"] = "true";
        doc["payload_not_available"] = "false";
        doc["brightness"] = true;
        doc["effect"] = true;
        doc["rgb"] = true;

        {
            JsonArray effectList = doc.createNestedArray("effect_list");
            iterateEnum<LedAnimationName>::iterate([&effectList](LedAnimationName, const char *name) {
                effectList.add(name);
            });
        }

        doc["schema"] = "json";
        doc["unique_id"] = fmt::format("{}_light", configs.hostname.value());
        fillCommonStuff(doc);

        std::string payload;
        serializeJson(doc, payload);

        publishQueue.push(std::make_tuple(
                fmt::format("{}light/{}/light/config", configs.hassMqttTopic.value(), configs.hostname.value()),
                payload));
    }

    mqttHassPublished = true;
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

        client.subscribe(fmt::format("{}/{}/set/#", configs.mqttTopic.value(), configs.hostname.value()).c_str(), 0);
        client.publish(fmt::format("{}/{}/online", configs.mqttTopic.value(), configs.hostname.value()).c_str(), "true", 0, 1);

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
        std::string topic{data->topic, static_cast<size_t>(data->topic_len)};
        std::string payload{data->data, static_cast<size_t>(data->data_len)};

        receiveQueue.push(std::make_tuple(topic, payload));
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

    static const std::string lastWillTopic = fmt::format("{}/{}/online", configs.mqttTopic.value(), configs.hostname.value());
    static constexpr const char * const lastWillMessage = "false";
    static constexpr const size_t lastWillMessageLen = std::char_traits<char>::length(lastWillMessage);

    client = {};
    lastMqttUrl = {};
    mqttState = MqttState::NotStarted;

    {
        const auto result = espcpputils::createTask(mqtt_handle_send, "mqttSend", 4096, nullptr, 5, nullptr,
                                                    espcpputils::CoreAffinity::Both);
        if (result != pdPASS)
        {
            auto msg = fmt::format("failed creating mqtt task {}", result);
            ESP_LOGE(TAG, "%.*s", msg.size(), msg.data());
            return;
        }
    }

    {
        const auto result = espcpputils::createTask(mqtt_handle_receive, "mqttReceive", 4096, nullptr, 5, nullptr,
                                                    espcpputils::CoreAffinity::Both);
        if (result != pdPASS)
        {
            auto msg = fmt::format("failed creating mqtt task {}", result);
            ESP_LOGE(TAG, "%.*s", msg.size(), msg.data());
            return;
        }
    }

    esp_mqtt_client_config_t mqtt_cfg{
            .broker = {
                    .address = {
                            .uri = url.data(),
                    },
            },
            .session = {
                    .last_will = {
                            .topic = lastWillTopic.c_str(),
                            .msg = lastWillMessage,
                            .msg_len = lastWillMessageLen,
                            .retain = 1,
                    }
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

    if (mqttState == MqttState::Connected && (!lastMqttPublish || espchrono::ago(*lastMqttPublish) > configs.mqttPublishInterval.value()))
    {
        publishStatus();
    }

    if (mqttState == MqttState::Connected && !mqttHassPublished)
    {
        publishHomeassistantDiscovery();
    }
}

} // namespace mqtt
