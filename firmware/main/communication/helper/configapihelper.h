#pragma once

// system includes
#include <string>
#include <optional>

namespace webserver {

struct ConfigApiGetResult
{
    std::optional<std::string> result;
    std::optional<std::string> error;
    const char* lastKey{nullptr};
    bool isLastKey{false};
};

struct ConfigApiSetResult
{
    bool success{false};
    std::optional<std::string> error; // json string {"success":false,"message":"error message"}
    std::optional<std::string> result;
};

const ConfigApiGetResult* getConfigAsJson(const char* lastKey);

const ConfigApiSetResult* setConfigFromJsonViaQuery(const std::string& requestQuery);

const ConfigApiSetResult* setConfigFromJsonViaBody(const std::string& requestBody);

} // namespace webserver
