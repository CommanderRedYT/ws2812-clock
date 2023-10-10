#pragma once

// system includes
#include <string>
#include <functional>

// esp-idf includes
#include <esp_err.h>

// 3rdparty lib includes
#include <ArduinoJson.h>

namespace status {

esp_err_t generateStatusJson(JsonObject& statusObj);

esp_err_t generateStatusJson(JsonDocument& statusObj);

esp_err_t generateStatusJson(std::string& json);

esp_err_t forEveryKey(const std::function<void(const JsonString&, const JsonVariant&)>& callback);

} // namespace status
