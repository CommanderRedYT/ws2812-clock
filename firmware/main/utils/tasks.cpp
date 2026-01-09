#include "tasks.h"

constexpr const char * const TAG = "tasks";

// 3rdparty lib includes
#include <espchrono.h>

// local includes
#include "communication/mdns_clock.h"
#include "communication/mqtt.h"
#include "communication/ota.h"
#include "communication/webserver.h"
#include "communication/wifi.h"
#include "espclock.h"
#include "peripheral/basicleds.h"
#include "peripheral/beeper.h"
#include "peripheral/ledmanager.h"

// optional local includes
#ifdef HARDWARE_USE_BME280
#include "peripheral/bme280.h"
#endif

namespace {

void noop() {}

using namespace espcpputils;
using namespace std::chrono_literals;

SchedulerTask tasksArray[]{
    SchedulerTask{"wifi",      wifi::begin,          wifi::update,      300ms},
    SchedulerTask{"mdns",      mdns::begin,          mdns::update,      300ms},
#ifdef HARDWARE_USE_BME280
    SchedulerTask{"bme280",    bme280_sensor::begin, noop,                 1s},
#endif
    SchedulerTask{"led",       ledmanager::begin,    ledmanager::update, 8ms},
    SchedulerTask{"basicleds", basicleds::begin,     basicleds::update,  60ms},
    SchedulerTask{"espclock",  espclock::begin,      espclock::update,  100ms},
    SchedulerTask{"webserver", webserver::begin,     noop,                 1s},
    SchedulerTask{"beeper",    beeper::begin,        beeper::update,     16ms},
    SchedulerTask{"mqtt",      mqtt::begin,          mqtt::update,      500ms},
    SchedulerTask{"ota",       ota::begin,           ota::update,       100ms},
};

} // namespace

cpputils::ArrayView<SchedulerTask> tasks{tasksArray};

void sched_pushStats(const bool printTasks)
{
    if (printTasks)
        ESP_LOGI(TAG, "begin listing tasks...");

    int totalMillis{};

    for (auto &task : tasks)
    {
        task.pushStats(printTasks);
        totalMillis += task.totalElapsed() / 1ms;
    }

    if (printTasks)
        ESP_LOGI(TAG, "end listing tasks. total sum of all tasks: %ims", totalMillis);
}
