#include "webserver_api.h"

constexpr const char * const TAG = "webserver_api";

// system includes
#include <memory>

// esp-idf includes
#include <esp_app_desc.h>
#include <esp_http_server.h>
#include <esp_log.h>

// 3rdparty lib includes
#include <esphttpdutils.h>
#include <makearray.h>
#include <recursivelockhelper.h>

// local includes
#include "communication/ota.h"
#include "helper/configapihelper.h"
#include "helper/status.h"
#include "peripheral/bme280.h"
#include "peripheral/ledhelpers/ledanimation.h"
#include "peripheral/ledmanager.h"
#include "utils/global_lock.h"
#include "utils/tasks.h"

using namespace std::chrono_literals;

namespace webserver {

namespace {
std::weak_ptr<ApiJsonDocument> _doc;
} // namespace

std::shared_ptr<ApiJsonDocument> getApiJson()
{
    if (const auto doc = _doc.lock())
        return doc;

    auto doc_ptr = std::make_shared<ApiJsonDocument>();
    _doc = doc_ptr;
    return doc_ptr;
}

namespace {

esp_err_t cors_handler(httpd_req_t* req)
{
    /*
    if (const auto res = httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*"); res != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to set Access-Control-Allow-Origin header: %s", esp_err_to_name(res));
        return res;
    }
    else
    {
        ESP_LOGI(TAG, "Set Access-Control-Allow-Origin header to *");
    }
    */

    if (const auto res = httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "http://localhost:3000"); res !=
                                                                                                          ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to set Access-Control-Allow-Origin header: %s", esp_err_to_name(res));
        return res;
    }
    else
    {
        ESP_LOGI(TAG, "Set Access-Control-Allow-Origin header to http://localhost:3000");
    }

    if (const auto res = httpd_resp_set_hdr(req, "Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
            res != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to set Access-Control-Allow-Methods header: %s", esp_err_to_name(res));
        return res;
    }
    else
    {
        ESP_LOGI(TAG, "Set Access-Control-Allow-Methods header to GET, POST, PUT, DELETE, OPTIONS");
    }

    if (const auto res = httpd_resp_set_hdr(req, "Access-Control-Allow-Headers", "Content-Type"); res != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to set Access-Control-Allow-Headers header: %s", esp_err_to_name(res));
        return res;
    }
    else
    {
        ESP_LOGI(TAG, "Set Access-Control-Allow-Headers header to Content-Type");
    }

    return ESP_OK;
}

esp_err_t api_get_config_handler(httpd_req_t* req)
{
    ESP_LOGI(TAG, "GET /api/config");

    espcpputils::RecursiveLockHelper lockHelper{global::global_lock->handle};

    if (const auto res = cors_handler(req); res != ESP_OK)
        return res;

    if (const auto res = httpd_resp_set_type(req, "application/json"); res != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to set content type: %s", esp_err_to_name(res));
        return res;
    }

    const char* lastKey = nullptr;

    bool error{false};

    while (true)
    {
        const auto* config = getConfigAsJson(lastKey);
        if (config == nullptr)
        {
            ESP_LOGE(TAG, "Failed to get config as JSON");
            break;
        }

        if (config->error)
        {
            ESP_LOGE(TAG, "Failed to get config as JSON: %s", config->error->c_str());
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, config->error->c_str());
            error = true;
            break;
        }

        if (!config->result)
            break;

        if (const auto res = httpd_resp_send_chunk(req, config->result->data(), config->result->size()); res != ESP_OK)
        {
            ESP_LOGE(TAG, "Failed to send chunk: %s", esp_err_to_name(res));
            return res;
        }

        lastKey = config->lastKey;

        if (config->isLastKey)
        {
            ESP_LOGI(TAG, "Last key reached");
            break;
        }
    }

    if (const auto res = httpd_resp_send_chunk(req, nullptr, 0); res != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to send chunk: %s", esp_err_to_name(res));
        return res;
    }

    return error ? ESP_FAIL : ESP_OK;
}

esp_err_t api_set_via_get_handler(httpd_req_t* req)
{
    ESP_LOGI(TAG, "GET /api/set");

    espcpputils::RecursiveLockHelper lockHelper{global::global_lock->handle};

    if (const auto res = cors_handler(req); res != ESP_OK)
        return res;

    // check if request contains a query string
    std::string query;
    if (auto result = esphttpdutils::webserver_get_query(req))
        query = *result;
    else
    {
        ESP_LOGE(TAG, "%.*s", result.error().size(), result.error().data());
        if (const auto res = esphttpdutils::webserver_resp_send(req, esphttpdutils::ResponseStatus::BadRequest, "application/json", result.error()); res != ESP_OK)
            ESP_LOGE(TAG, "Failed to send response: %s", esp_err_to_name(res));
        return ESP_FAIL;
    }

    if (const auto result = setConfigFromJsonViaQuery(query); result && result->success)
    {
        if (const auto res = esphttpdutils::webserver_resp_send(req, esphttpdutils::ResponseStatus::Ok, "application/json", result->result.value()); res != ESP_OK)
        {
            ESP_LOGE(TAG, "Failed to send response: %s", esp_err_to_name(res));
            return res;
        }
    }
    else if (result && result->error)
    {
        ESP_LOGE(TAG, "%.*s", result->error->size(), result->error->data());
        if (const auto res = esphttpdutils::webserver_resp_send(req, esphttpdutils::ResponseStatus::BadRequest, "application/json", result->error.value()); res != ESP_OK)
            ESP_LOGE(TAG, "Failed to send response: %s", esp_err_to_name(res));
        return ESP_FAIL;
    }
    else
    {
        ESP_LOGE(TAG, "Failed to set config from JSON");
        if (const auto res = esphttpdutils::webserver_resp_send(req, esphttpdutils::ResponseStatus::InternalServerError, "application/json", R"({"success": false,"message:"Failed to set config from JSON")"); res != ESP_OK)
            ESP_LOGE(TAG, "Failed to send response: %s", esp_err_to_name(res));
        return ESP_FAIL;
    }

    if (const auto res = esphttpdutils::webserver_resp_send(req, esphttpdutils::ResponseStatus::Ok, "text/plain", "Ok"); res != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to send response: %s", esp_err_to_name(res));
        return res;
    }

    return ESP_OK;
}

esp_err_t api_set_via_post_handler(httpd_req_t* req)
{
    ESP_LOGI(TAG, "POST /api/set");

    espcpputils::RecursiveLockHelper lockHelper{global::global_lock->handle};

    if (const auto res = cors_handler(req); res != ESP_OK)
        return res;

    std::string body;
    if (httpd_req_get_hdr_value_len(req, "Content-Length") != 0)
    {
        char contentType[32];
        if (const auto res = httpd_req_get_hdr_value_str(req, "Content-Type", contentType, 32); res != ESP_OK)
        {
            ESP_LOGE(TAG, "Failed to get Content-Type header: %s", esp_err_to_name(res));
            if (const auto resp_res = esphttpdutils::webserver_resp_send(req, esphttpdutils::ResponseStatus::BadRequest, "application/json", fmt::format(R"({{"success":false,"message":"Failed to get Content-Type header: {}"}})", esp_err_to_name(res))); resp_res != ESP_OK)
                ESP_LOGE(TAG, "Failed to send response: %s", esp_err_to_name(resp_res));
            return ESP_FAIL;
        }
        else
        {
            if (strcmp(contentType, "application/json") != 0)
            {
                ESP_LOGE(TAG, "Invalid Content-Type: %s", contentType);
                if (const auto resp_res = esphttpdutils::webserver_resp_send(req, esphttpdutils::ResponseStatus::BadRequest, "application/json", fmt::format(R"({{"success":false,"message":"Invalid Content-Type: {}"}})", contentType)); resp_res != ESP_OK)
                    ESP_LOGE(TAG, "Failed to send response: %s", esp_err_to_name(resp_res));
                return ESP_FAIL;
            }
        }

        // if it does we allocate a buffer to store the body
        const auto contentLength = httpd_req_get_hdr_value_len(req, "Content-Length") + 1;
        char* buffer = static_cast<char*>(malloc(contentLength));
        if (buffer == nullptr)
        {
            ESP_LOGE(TAG, "Failed to allocate buffer for body");
            if (const auto res = esphttpdutils::webserver_resp_send(req, esphttpdutils::ResponseStatus::InternalServerError, "application/json", R"({"success":false,"message":"Failed to allocate buffer for body"})"); res != ESP_OK)
                ESP_LOGE(TAG, "Failed to send response: %s", esp_err_to_name(res));
            return ESP_FAIL;
        }
        // then we read the body into the buffer
        const auto readBytes = httpd_req_recv(req, buffer, contentLength - 1);
        if (readBytes == HTTPD_SOCK_ERR_TIMEOUT || readBytes == HTTPD_SOCK_ERR_FAIL)
        {
            ESP_LOGE(TAG, "Failed to read body");
            if (const auto res = esphttpdutils::webserver_resp_send(req, esphttpdutils::ResponseStatus::InternalServerError, "application/json", R"({"success":false,"message":"Failed to read body"})"); res != ESP_OK)
                ESP_LOGE(TAG, "Failed to send response: %s", esp_err_to_name(res));
            return ESP_FAIL;
        }
        // we have to add a null terminator to the buffer
        buffer[readBytes] = '\0';
        // and finally we store the buffer in a string
        body = buffer;
        // and free the buffer
        free(buffer);
    }
    else
    {
        ESP_LOGE(TAG, "Request does not contain a body");
        if (const auto res = esphttpdutils::webserver_resp_send(req, esphttpdutils::ResponseStatus::BadRequest, "application/json", R"({"success":false,"message":"Request does not contain a body"})"); res != ESP_OK)
            ESP_LOGE(TAG, "Failed to send response: %s", esp_err_to_name(res));
        return ESP_FAIL;
    }

    if (const auto result = setConfigFromJsonViaBody(body); result && result->success)
    {
        if (const auto res = esphttpdutils::webserver_resp_send(req, esphttpdutils::ResponseStatus::Ok, "application/json", result->result.value()); res != ESP_OK)
        {
            ESP_LOGE(TAG, "Failed to send response: %s", esp_err_to_name(res));
            return res;
        }

        return ESP_OK;
    }
    else if (result && result->error)
    {
        ESP_LOGE(TAG, "%.*s", result->error->size(), result->error->data());
        if (const auto res = esphttpdutils::webserver_resp_send(req, esphttpdutils::ResponseStatus::BadRequest, "application/json", result->error.value()); res != ESP_OK)
            ESP_LOGE(TAG, "Failed to send response: %s", esp_err_to_name(res));
        return ESP_FAIL;
    }
    else
    {
        ESP_LOGE(TAG, "Failed to set config from JSON");
        if (const auto res = esphttpdutils::webserver_resp_send(req, esphttpdutils::ResponseStatus::InternalServerError, "application/json", R"({"success":false,"message":"Failed to set config from JSON"})"); res != ESP_OK)
            ESP_LOGE(TAG, "Failed to send response: %s", esp_err_to_name(res));
        return ESP_FAIL;
    }
}

esp_err_t api_get_leds_handler(httpd_req_t* req)
{
    ESP_LOGD(TAG, "GET /api/leds");

    espcpputils::RecursiveLockHelper lockHelper{global::global_lock->handle};

    if (const auto res = cors_handler(req); res != ESP_OK)
        return res;

    if (const auto res = httpd_resp_set_type(req, "application/json"); res != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to set content type: %s", esp_err_to_name(res));
        return res;
    }

    const auto& leds = ledmanager::getLeds();

    if (const auto res = httpd_resp_send_chunk(req, "[", 1); res != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to send response: %s", esp_err_to_name(res));
        return res;
    }

    for (auto i = 0; i < leds.size(); ++i)
    {
        const auto& led = leds[i];
        const auto json = fmt::format("[{},{},{}]{}", led.red, led.green, led.blue, i < leds.size() - 1 ? "," : "");
        if (const auto res = httpd_resp_send_chunk(req, json.c_str(), json.size()); res != ESP_OK)
        {
            ESP_LOGE(TAG, "Failed to send response: %s", esp_err_to_name(res));
            return res;
        }
    }

    if (const auto res = httpd_resp_send_chunk(req, "]", 1); res != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to send response: %s", esp_err_to_name(res));
        return res;
    }

    if (const auto res = httpd_resp_send_chunk(req, nullptr, 0); res != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to send response: %s", esp_err_to_name(res));
        return res;
    }

    return ESP_OK;
}

esp_err_t api_get_status_handler(httpd_req_t* req)
{
    ESP_LOGI(TAG, "GET /api/status");
    espcpputils::RecursiveLockHelper lockHelper{global::global_lock->handle};

    if (const auto res = cors_handler(req); res != ESP_OK)
        return res;

    std::string json;

    if (const auto status = status::generateStatusJson(json); status != ESP_OK)
    {
        if (const auto res = esphttpdutils::webserver_resp_send(req, esphttpdutils::ResponseStatus::InternalServerError, "application/json", R"({"success":false,"message":"Failed to serialize JSON"})"); res != ESP_OK)
        {
            ESP_LOGE(TAG, "Failed to send response: %s", esp_err_to_name(res));
            return res;
        }
    }

    if (const auto res = esphttpdutils::webserver_resp_send(req, esphttpdutils::ResponseStatus::Ok, "application/json", json); res != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to send response: %s", esp_err_to_name(res));
        return res;
    }

    return ESP_OK;
}

esp_err_t api_get_tasks_handler(httpd_req_t* req)
{
    ESP_LOGI(TAG, "GET /api/tasks");

    espcpputils::RecursiveLockHelper lockHelper{global::global_lock->handle};

    if (const auto res = cors_handler(req); res != ESP_OK)
        return res;

    auto guard = getApiJson();
    auto& doc = *guard;

    doc.clear();

    doc["success"] = true;

    doc.set(JsonArray{}); // create empty array

    for (auto& task : tasks)
    {
        auto taskObj = doc.createNestedObject();

        taskObj["name"] = task.name();
        taskObj["count"] = task.callCount();
        taskObj["last"] = std::chrono::floor<std::chrono::milliseconds>(task.lastElapsed()).count();
        taskObj["avg"] = std::chrono::floor<std::chrono::milliseconds>(task.averageElapsed()).count();
        taskObj["max"] = std::chrono::floor<std::chrono::milliseconds>(task.maxElapsed()).count();
    }

    if (doc.overflowed())
    {
        if (const auto res = esphttpdutils::webserver_resp_send(req, esphttpdutils::ResponseStatus::InternalServerError, "application/json", R"({"success":false,"message":"Failed to serialize JSON"})"); res != ESP_OK)
        {
            ESP_LOGE(TAG, "Failed to send response: %s", esp_err_to_name(res));
            return res;
        }
        return ESP_FAIL;
    }

    std::string json;

    serializeJson(doc, json);

    if (const auto res = esphttpdutils::webserver_resp_send(req, esphttpdutils::ResponseStatus::Ok, "application/json", json); res != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to send response: %s", esp_err_to_name(res));
        return res;
    }

    return ESP_OK;
}

esp_err_t api_get_animations_handler(httpd_req_t* req)
{
    ESP_LOGI(TAG, "GET /api/animations");

    espcpputils::RecursiveLockHelper lockHelper{global::global_lock->handle};

    if (const auto res = cors_handler(req); res != ESP_OK)
        return res;

    auto guard = getApiJson();
    auto& doc = *guard;

    doc.clear();

    doc["success"] = true;

    doc.set(JsonArray{}); // create empty array

    for (auto& animation : animation::animations)
    {
        auto animationObj = doc.createNestedObject();

        animationObj["name"] = animation->getName();
    }

    if (doc.overflowed())
    {
        if (const auto res = esphttpdutils::webserver_resp_send(req, esphttpdutils::ResponseStatus::InternalServerError, "application/json", R"({"success":false,"message":"Failed to serialize JSON"})"); res != ESP_OK)
        {
            ESP_LOGE(TAG, "Failed to send response: %s", esp_err_to_name(res));
            return res;
        }
        return ESP_FAIL;
    }

    std::string json;

    serializeJson(doc, json);

    if (const auto res = esphttpdutils::webserver_resp_send(req, esphttpdutils::ResponseStatus::Ok, "application/json", json); res != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to send response: %s", esp_err_to_name(res));
        return res;
    }

    return ESP_OK;
}

esp_err_t api_get_ota_status_handler(httpd_req_t* req)
{
    ESP_LOGI(TAG, "GET /api/ota/status");

    espcpputils::RecursiveLockHelper lockHelper{global::global_lock->handle};

    if (const auto res = cors_handler(req); res != ESP_OK)
        return res;

    auto guard = getApiJson();
    auto& doc = *guard;

    doc.clear();

    doc["success"] = true;

    doc["isInProgress"] = ota::isInProgress();
    doc["otaMessage"] = ota::otaMessage();
    doc["isConstructed"] = ota::isConstructed();
    doc["progress"] = ota::progress();
    doc["percentage"] = ota::percent();
    if (const auto res = ota::totalSize(); res)
        doc["totalSize"] = *res;
    else
        doc["totalSize"] = nullptr;

    if (const auto res = esp_app_get_description(); res)
    {
        auto& appDesc = *res;
        auto currentAppObj = doc.createNestedObject("currentApp");

        currentAppObj["version"] = appDesc.version;
        // currentAppObj["app_elf_sha256"] = appDesc.app_elf_sha256;
        currentAppObj["date"] = appDesc.date;
        currentAppObj["idf_ver"] = appDesc.idf_ver;
        currentAppObj["magic_word"] = appDesc.magic_word;
        currentAppObj["project_name"] = appDesc.project_name;
        currentAppObj["secure_version"] = appDesc.secure_version;
        currentAppObj["time"] = appDesc.time;
    }
    else
    {
        doc["currentApp"] = nullptr;
    }

    if (ota::otherAppDesc)
    {
        auto& appDesc = *ota::otherAppDesc;
        auto currentAppObj = doc.createNestedObject("otherApp");

        currentAppObj["version"] = appDesc.version;
        // currentAppObj["app_elf_sha256"] = appDesc.app_elf_sha256;
        currentAppObj["date"] = appDesc.date;
        currentAppObj["idf_ver"] = appDesc.idf_ver;
        currentAppObj["magic_word"] = appDesc.magic_word;
        currentAppObj["project_name"] = appDesc.project_name;
        currentAppObj["secure_version"] = appDesc.secure_version;
        currentAppObj["time"] = appDesc.time;
    }
    else
    {
        doc["otherApp"] = nullptr;
    }

    if (doc.overflowed())
    {
        if (const auto res = esphttpdutils::webserver_resp_send(req, esphttpdutils::ResponseStatus::InternalServerError, "application/json", R"({"success":false,"message":"Failed to serialize JSON"})"); res != ESP_OK)
        {
            ESP_LOGE(TAG, "Failed to send response: %s", esp_err_to_name(res));
            return res;
        }
        return ESP_FAIL;
    }

    std::string json;

    serializeJson(doc, json);

    if (const auto res = esphttpdutils::webserver_resp_send(req, esphttpdutils::ResponseStatus::Ok, "application/json", json); res != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to send response: %s", esp_err_to_name(res));
        return res;
    }

    return ESP_OK;
}

esp_err_t api_trigger_ota_handler(httpd_req_t* req)
{
    ESP_LOGI(TAG, "POST /api/ota/trigger");

    espcpputils::RecursiveLockHelper lockHelper{global::global_lock->handle};

    if (const auto res = cors_handler(req); res != ESP_OK)
        return res;

    std::string query;
    if (auto result = esphttpdutils::webserver_get_query(req))
        query = *result;
    else
    {
        ESP_LOGE(TAG, "%.*s", result.error().size(), result.error().data());
        if (const auto res = esphttpdutils::webserver_resp_send(req, esphttpdutils::ResponseStatus::BadRequest, "application/json", fmt::format("{{\"success\":false,\"message\":\"{}\"}}", result.error())); res != ESP_OK)
            ESP_LOGE(TAG, "Failed to send response: %s", esp_err_to_name(res));
        return ESP_FAIL;
    }

    std::string url;
    constexpr const std::string_view urlParamName = "url";

    {
        char valueBufEncoded[256];
        if (const auto result = httpd_query_key_value(query.data(), urlParamName.data(), valueBufEncoded, 256); result != ESP_OK)
        {
            if (result == ESP_ERR_NOT_FOUND)
            {
                const auto msg = fmt::format(R"({{"success":false,"message":"Missing required parameter '{}'"}})", urlParamName);
                ESP_LOGW(TAG, "%.*s", msg.size(), msg.data());
                if (const auto res = esphttpdutils::webserver_resp_send(req, esphttpdutils::ResponseStatus::BadRequest, "application/json", msg); res != ESP_OK)
                    ESP_LOGE(TAG, "Failed to send response: %s", esp_err_to_name(res));
                return ESP_FAIL;
            }
            else
            {
                const auto msg = fmt::format("{{\"success\":false,\"message\":\"Failed to get parameter '{}' ({})\"}})", urlParamName, esp_err_to_name(result));
                ESP_LOGE(TAG, "%.*s", msg.size(), msg.data());
                if (const auto res = esphttpdutils::webserver_resp_send(req, esphttpdutils::ResponseStatus::BadRequest, "application/json", msg); res != ESP_OK)
                    ESP_LOGE(TAG, "Failed to send response: %s", esp_err_to_name(res));
                return ESP_FAIL;
            }
        }

        char valueBuf[257];
        esphttpdutils::urldecode(valueBuf, valueBufEncoded);

        url = valueBuf;
    }

    if (const auto otaRes = ota::trigger(url); !otaRes)
    {
        const auto msg = fmt::format("{{\"success\":false,\"message\":\"Failed to trigger OTA ({})\"}})", otaRes.error());
        ESP_LOGE(TAG, "%.*s", msg.size(), msg.data());
        if (const auto res = esphttpdutils::webserver_resp_send(req, esphttpdutils::ResponseStatus::InternalServerError, "application/json", msg); res != ESP_OK)
            ESP_LOGE(TAG, "Failed to send response: %s", esp_err_to_name(res));
        return ESP_FAIL;
    }

    if (const auto res = esphttpdutils::webserver_resp_send(req, esphttpdutils::ResponseStatus::Ok, "application/json", R"({"success":true})"); res != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to send response: %s", esp_err_to_name(res));
        return res;
    }

    return ESP_OK;
}

esp_err_t api_trigger_reboot_handler(httpd_req_t* req)
{
    ESP_LOGI(TAG, "POST /api/reboot/trigger");

    espcpputils::RecursiveLockHelper lockHelper{global::global_lock->handle};

    if (const auto res = cors_handler(req); res != ESP_OK)
        return res;

    if (const auto res = esphttpdutils::webserver_resp_send(req, esphttpdutils::ResponseStatus::Ok, "application/json", R"({"success":true})"); res != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to send response: %s", esp_err_to_name(res));
        return res;
    }

    esp_restart();

    return ESP_OK;
}

auto get_handlers()
{
    return cpputils::make_array(
        httpd_uri_t{ .uri = "/api/v1/status",     .method = HTTP_GET,  .handler = api_get_status_handler,     .user_ctx = nullptr },
        httpd_uri_t{ .uri = "/api/v1/config",     .method = HTTP_GET,  .handler = api_get_config_handler,     .user_ctx = nullptr },
        httpd_uri_t{ .uri = "/api/v1/set",        .method = HTTP_GET,  .handler = api_set_via_get_handler,    .user_ctx = nullptr },
        httpd_uri_t{ .uri = "/api/v1/set",        .method = HTTP_POST, .handler = api_set_via_post_handler,   .user_ctx = nullptr },
        httpd_uri_t{ .uri = "/api/v1/leds",       .method = HTTP_GET,  .handler = api_get_leds_handler,       .user_ctx = nullptr },
        httpd_uri_t{ .uri = "/api/v1/tasks",      .method = HTTP_GET,  .handler = api_get_tasks_handler,      .user_ctx = nullptr },
        httpd_uri_t{ .uri = "/api/v1/animations", .method = HTTP_GET,  .handler = api_get_animations_handler, .user_ctx = nullptr },
        httpd_uri_t{ .uri = "/api/v1/ota",        .method = HTTP_GET,  .handler = api_get_ota_status_handler, .user_ctx = nullptr },
        httpd_uri_t{ .uri = "/api/v1/triggerOta", .method = HTTP_GET,  .handler = api_trigger_ota_handler,    .user_ctx = nullptr },
        httpd_uri_t{ .uri = "/api/v1/reboot",     .method = HTTP_GET,  .handler = api_trigger_reboot_handler, .user_ctx = nullptr }
    );
}

} // namespace

void webserver_api_setup(httpd_handle_t handle)
{
    for (const auto& handler : get_handlers())
    {
        ESP_LOGI(TAG, "Registering URI handler for %s", handler.uri);
        if (const auto res = httpd_register_uri_handler(handle, &handler); res != ESP_OK)
        {
            ESP_LOGE(TAG, "Failed to register URI handler for %s: %s", handler.uri, esp_err_to_name(res));
        }
    }
}

} // namespace webserver
