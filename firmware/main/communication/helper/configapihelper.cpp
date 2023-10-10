#include "configapihelper.h"

// esp-idf includes
#include <esp_log.h>

// 3rdparty lib includes
#include <ArduinoJson.h>
#include <fmt/core.h>
#include <recursivelockhelper.h>
#include <wrappers/recursive_mutex_semaphore.h>

// local includes
#include "communication/webserver_api.h"
#include "saveSetting.h"
#include "toJson.h"
#include "utils/config.h"

namespace webserver {

namespace {

// expose the JsonDocument constructor
class PublicJsonDocument : public JsonDocument
{
public:
    using JsonDocument::JsonDocument;

    PublicJsonDocument(char* buffer, size_t capacity) : JsonDocument{buffer, capacity} {}
};

constexpr const char *const TAG = "ConfigApiHelper";

ConfigApiGetResult configApiGetResult;
ConfigApiSetResult configApiSetResult;
espcpputils::recursive_mutex_semaphore configApiMutex{};

} // namespace

const ConfigApiGetResult* getConfigAsJson(const char* lastKey)
{
    espcpputils::RecursiveLockHelper lockHelper{configApiMutex.handle};

    auto guard = getApiJson();
    auto& doc = *guard;

    bool lastKeyReached{false};

    configApiGetResult.result = std::nullopt;
    configApiGetResult.lastKey = nullptr;
    configApiGetResult.isLastKey = false;

    doc.clear();

    doc.set(JsonObject{});

    // if lastKey is nullptr, do not skip any key
    std::unique_ptr<char[]> buf;

    ESP_LOGI(TAG, "lastkey=%s", lastKey == nullptr ? "nullptr" : lastKey);

    configs.callForEveryConfig([&](auto& config) {
        if (lastKey != nullptr && config.nvsName() != lastKey && !lastKeyReached)
            return false;

        if (lastKey != nullptr && config.nvsName() == lastKey)
        {
            ESP_LOGI(TAG, "lastKey reached");
            lastKeyReached = true;
        }

        ESP_LOGD(TAG, "Adding config %s", config.nvsName());

        buf = std::make_unique<char[]>(128);
        PublicJsonDocument currentValue{buf.get(), 128};

        if (const auto res = apihelpers::toJson(config.value(), currentValue); !res)
        {
            ESP_LOGE(TAG, "Failed to convert config %s to JSON", config.nvsName());
            configApiGetResult.error = res.error();
            return true;
        }

        doc[config.nvsName()] = currentValue;

        if (doc.overflowed())
        {
            ESP_LOGI(TAG, "Document overflowed, returning");
            configApiGetResult.isLastKey = false;
            return true;
        }

        configApiGetResult.lastKey = config.nvsName();
        configApiGetResult.isLastKey = true;

        return false;
    });

    buf.reset();

    if (configApiGetResult.error.has_value())
    {
        ESP_LOGE(TAG, "Failed to convert config to JSON: %s", configApiGetResult.error->c_str());
        return &configApiGetResult;
    }

    configApiGetResult.result = std::make_optional<std::string>();
    serializeJson(doc, *configApiGetResult.result);

    if (!configApiGetResult.isLastKey)
    {
        ESP_LOGI(TAG, "Returning partial result, removing trailing parts");
        configApiGetResult.result->pop_back(); // remove trailing '}'
        *configApiGetResult.result += ","; // add trailing ',' for next batch
    }

    if (lastKey != nullptr)
    {
        ESP_LOGI(TAG, "Returning partial result, removing previous parts");
        configApiGetResult.result->erase(0, 1);
    }

    return &configApiGetResult;
}

const ConfigApiSetResult* setConfigFromJsonViaQuery(const std::string& requestQuery)
{
    espcpputils::RecursiveLockHelper guard{configApiMutex.handle};

    configApiSetResult.result = std::nullopt;
    configApiSetResult.success = true;
    configApiSetResult.error = std::nullopt;

    std::string successResult = R"({"success":true, "keys": [)";

    bool successfullySetKey{false};

    configs.callForEveryConfig([&](auto& config){
        const std::string_view nvsName{config.nvsName()};
        char valueBufEncoded[256];

        if (const auto res = httpd_query_key_value(requestQuery.data(), nvsName.data(), valueBufEncoded, sizeof(valueBufEncoded)); res != ESP_ERR_NOT_FOUND && res != ESP_OK)
        {
            // { "success": false, "message": "..." }
            configApiSetResult.error = fmt::format("{{\"success\":false, \"message\": \"Failed to get value (nvsName={} err={} requestQuery={})\"}}", nvsName, esp_err_to_name(res), requestQuery);
            configApiSetResult.success = false;
            return false;
        }
        else if (res == ESP_ERR_NOT_FOUND)
        {
            return false;
        }

        char valueBuf[257];
        esphttpdutils::urldecode(valueBuf, valueBufEncoded);

        if (const auto res = saveSetting(config, valueBuf); !res)
        {
            // { "success": false, "message": "..." }
            if (!successfullySetKey)
                configApiSetResult.error = fmt::format("{{\"success\":false, \"message\": \"Failed to save value for key {} ({})\", \"keys\": []}}", nvsName, res.error());
            else
                configApiSetResult.error = fmt::format("{{\"success\":false, \"message\": \"Failed to save value for key {} ({})\", \"keys\": {}]}}", nvsName, res.error(), successResult);
            configApiSetResult.success = false;
            return true;
        }
        else
        {
            // { "success": true, "message": "..." }
            successResult += fmt::format(R"({{"key": "{}", "value": "{}"}},)", nvsName, valueBuf);
            successfullySetKey = true;
            configApiSetResult.success = true;
            configApiSetResult.error = std::nullopt;
            return false;
        }
    });

    if (successfullySetKey)
    {
        successResult.pop_back(); // remove trailing ','
        successResult += "]}";
        configApiSetResult.result = successResult;
    }
    else if (!configApiSetResult.error.has_value())
    {
        configApiSetResult.error = R"({"success":false, "message": "No keys were set"})";
        configApiSetResult.success = false;
    }
    else
    {
        configApiSetResult.success = false;
    }

    return &configApiSetResult;
}

const ConfigApiSetResult* setConfigFromJsonViaBody(const std::string& requestBody)
{
    // { "<key>": "<value>", ... }
    espcpputils::RecursiveLockHelper guard{configApiMutex.handle};

    std::string successResult = R"({"success":true, "keys": [)";

    // use ArduinoJson to parse body and do the above
    StaticJsonDocument<512> doc;

    if (const auto res = deserializeJson(doc, requestBody); res != DeserializationError::Ok)
    {
        configApiSetResult.error = fmt::format("{{\"success\":false, \"message\": \"Failed to parse JSON body ({})\"}}", res.c_str());
        configApiSetResult.success = false;
        return &configApiSetResult;
    }

    bool successfullySetKey{false};

    /*for (const auto& kv : doc.as<JsonObject>())
    {
        const std::string_view nvsName{kv.key().c_str()};
        const std::string_view value{kv.value().as<std::string>()};

        configs.callForEveryConfig([&](auto& config) {
            if (config.nvsName() != nvsName)
                return false;

            if (const auto res = saveSetting(config, value); !res)
            {
                // { "success": false, "message": "..." }
                if (!successfullySetKey)
                    configApiSetResult.error = fmt::format(
                            "{{\"success\":false, \"message\": \"Failed to save value for key {} ({})\", \"keys\": []}}",
                            nvsName, res.error());
                else
                    configApiSetResult.error = fmt::format(
                            "{{\"success\":false, \"message\": \"Failed to save value for key {} ({})\", \"keys\": {}]}}",
                            nvsName, res.error(), successResult);
                configApiSetResult.success = false;
                return true;
            }
            else
            {
                // { "success": true, "message": "..." }
                successResult += fmt::format(R"("{{"key": "{}", "value": "{}"}}",)", nvsName, value);
                successfullySetKey = true;
                return true;
            }
        });

        if (!configApiSetResult.success)
            return &configApiSetResult;
    }*/

    const auto& obj = doc.as<JsonObject>();

    configs.callForEveryConfig([&](auto& config) {
        const std::string_view nvsName{config.nvsName()};
        const auto& value = obj[nvsName.data()];

        if (value.isNull())
            return false;

        if (const auto res = saveSetting(config, value.as<std::string>()); !res)
        {
            // { "success": false, "message": "..." }
            if (!successfullySetKey)
                configApiSetResult.error = fmt::format(
                        "{{\"success\":false, \"message\": \"Failed to save value for key {} ({})\", \"keys\": []}}",
                        nvsName, res.error());
            else
                configApiSetResult.error = fmt::format(
                        "{{\"success\":false, \"message\": \"Failed to save value for key {} ({})\", \"keys\": {}]}}",
                        nvsName, res.error(), successResult);
            configApiSetResult.success = false;
            return true;
        }
        else
        {
            // { "success": true, "message": "..." }
            successResult += fmt::format(R"({{"key": "{}", "value": "{}"}},)", nvsName, value.as<std::string>());
            successfullySetKey = true;
            return false;
        }
    });

    return &configApiSetResult;
}

} // namespace webserver
