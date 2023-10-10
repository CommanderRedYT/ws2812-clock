#pragma once

// 3rdparty lib includes
#include <wrappers/mqtt_client.h>

namespace mqtt {

namespace topics {

constexpr const char * const status = "online"; // Content: "online" or "offline"
constexpr const char * const config = "status"; // Content: JSON object

} // namespace topics

extern espcpputils::mqtt_client client;

void begin();

void update();

} // namespace mqtt
