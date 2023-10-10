#pragma once

// system includes
#include <memory>

// esp-idf includes
#include <esp_http_server.h>

// 3rdparty lib includes
#include <ArduinoJson.h>

namespace webserver {

constexpr const auto API_JSON_SIZE = 2048;

using ApiJsonDocument = StaticJsonDocument<API_JSON_SIZE>;

std::shared_ptr<ApiJsonDocument> getApiJson();

void webserver_api_setup(httpd_handle_t handle);

} // namespace webserver
