#pragma once

// esp-idf includes
#include <esp_http_server.h>

// 3rdparty lib includes
#include <delayedconstruction.h>
#include <wrappers/recursive_mutex_semaphore.h>

namespace webserver {

extern httpd_handle_t httpdHandle;

void begin();

} // namespace webserver
