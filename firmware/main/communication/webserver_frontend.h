#pragma once

// esp-idf includes
#include <esp_http_server.h>

namespace webserver {

esp_err_t captive_portal_handler(httpd_req_t *req);

void webserver_frontend_setup(httpd_handle_t handle);

} // namespace webserver
