#include "webserver.h"

constexpr const char * const TAG = "webserver";

// esp-idf includes
#include <esp_http_server.h>
#include <esp_log.h>

// 3rdparty lib includes
#include <recursivelockhelper.h>

// local includes
#include "webserver_api.h"
#include "webserver_frontend.h"

namespace webserver {

httpd_handle_t httpdHandle;

void begin()
{
    httpd_config_t httpdConfig = HTTPD_DEFAULT_CONFIG();
    httpdConfig.core_id = 1;
    httpdConfig.max_uri_handlers = 64;
    httpdConfig.stack_size = 8192;

    if (const auto result = httpd_start(&httpdHandle, &httpdConfig); result != ESP_OK)
    {
        ESP_LOGE(TAG, "httpd_start(): %s", esp_err_to_name(result));
        return;
    }

    webserver_api_setup(httpdHandle);
    webserver_frontend_setup(httpdHandle);
}

} // namespace webserver
