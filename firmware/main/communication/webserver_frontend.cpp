#include "webserver_frontend.h"

constexpr const char * const TAG = "webserver_frontend";

// esp-idf includes
#include <esp_log.h>
#include <lwip/inet.h>
#include <lwip/sockets.h>

// 3rdparty lib includes
#include <fmt/format.h>

// code-gen
#include "webserver_files.h"

// local includes
#include "utils/config.h"

namespace webserver {

esp_err_t captive_portal_handler(httpd_req_t *req)
{
    const auto handler = reinterpret_cast<esp_err_t(*)(httpd_req_t*)>(req->user_ctx);

    ESP_LOGI(TAG, "captive_portal_handler()");

    char host[32];

    if (const auto res = httpd_req_get_hdr_value_str(req, "Host", host, sizeof(host)); res != ESP_OK)
    {
        ESP_LOGE(TAG, "httpd_req_get_hdr_value_str(): %s", esp_err_to_name(res));
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Host: %s", host);

    // if host is not the same as the AP IP, serve the captive portal
    if (host != toString(configs.wifiApIp.value()))
    {
        // redirect to the captive portal
        const auto redirectUrl = fmt::format("http://{}/", toString(configs.wifiApIp.value()));
        ESP_LOGI(TAG, "Redirecting to %s", redirectUrl.c_str());
        httpd_resp_set_hdr(req, "Location", redirectUrl.c_str());
        httpd_resp_set_status(req, "302 Found");
        httpd_resp_send(req, nullptr, 0);

        return ESP_OK;
    }

    return handler(req);
}

esp_err_t handle_not_found(httpd_req_t* req, httpd_err_code_t error)
{
    ESP_LOGW(TAG, "handle_not_found(): %d", error);
    // redirect to /
    const auto redirectUrl = fmt::format("http://{}/", toString(configs.wifiApIp.value()));
    ESP_LOGI(TAG, "Redirecting to %s", redirectUrl.c_str());
    if (const auto res = httpd_resp_set_hdr(req, "Location", redirectUrl.c_str()); res != ESP_OK)
    {
        ESP_LOGE(TAG, "httpd_resp_set_hdr(): %s", esp_err_to_name(res));
        return res;
    }

    if (const auto res = httpd_resp_set_status(req, "204 No Content"); res != ESP_OK)
    {
        ESP_LOGE(TAG, "httpd_resp_set_status(): %s", esp_err_to_name(res));
        return res;
    }

    if (const auto res = httpd_resp_send(req, nullptr, 0); res != ESP_OK)
    {
        ESP_LOGE(TAG, "httpd_resp_send(): %s", esp_err_to_name(res));
        return res;
    }

    return ESP_OK;
}

void webserver_frontend_setup(httpd_handle_t handle)
{
    for (const auto& handler : webserver_files::get_handlers())
    {
        ESP_LOGI(TAG, "Registering URI handler for %s", handler.uri);
        if (const auto res = httpd_register_uri_handler(handle, &handler); res != ESP_OK)
        {
            ESP_LOGE(TAG, "Failed to register URI handler for %s: %s", handler.uri, esp_err_to_name(res));
        }
    }

    /*
    if (const auto res = httpd_register_err_handler(handle, HTTPD_404_NOT_FOUND, handle_not_found); res != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to register 404 handler: %s", esp_err_to_name(res));
    }
    */
}

} // namespace webserver
