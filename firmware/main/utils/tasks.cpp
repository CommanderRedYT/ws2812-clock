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
    SchedulerTask{"wifi",      wifi::begin,          wifi::update,      100ms},
    SchedulerTask{"mdns",      mdns::begin,          mdns::update,      100ms},
#ifdef HARDWARE_USE_BME280
    SchedulerTask{"bme280",    bme280_sensor::begin, noop,                 1s},
#endif
    SchedulerTask{"led",       ledmanager::begin,    ledmanager::update,  1ms},
    SchedulerTask{"basicleds", basicleds::begin,     basicleds::update,  10ms},
    SchedulerTask{"espclock",  espclock::begin,      espclock::update,   10ms},
    SchedulerTask{"webserver", webserver::begin,     noop              , 50ms},
    SchedulerTask{"beeper",    beeper::begin,        beeper::update,     20ms},
    SchedulerTask{"mqtt",      mqtt::begin,          mqtt::update,      500ms},
    SchedulerTask{"ota",       ota::begin,           ota::update,       100ms},
};

} // namespace

cpputils::ArrayView<espcpputils::SchedulerTask> tasks{tasksArray};

void sched_pushStats(bool printTasks)
{
    if (printTasks)
        ESP_LOGI(TAG, "begin listing tasks...");

    for (auto &task : tasks)
        task.pushStats(printTasks);

    if (printTasks)
        ESP_LOGI(TAG, "end listing tasks");
}
