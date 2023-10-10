#include "basicleds.h"

constexpr const char * const TAG = "BASICLEDS";

// esp-idf includes
#include <driver/gpio.h>

// 3rdparty lib includes
#include <espchrono.h>

// local includes
#include "communication/wifi.h"
#include "utils/config.h"

namespace basicleds {

LedConfig ledConfig;

void begin()
{
    // PIN: HARDWARE_AP_STATUS_LED_PIN
    // PIN: HARDWARE_STA_STATUS_LED_PIN
    // PIN: HARDWARE_ALARM_STATUS_LED_PIN


}

void update()
{

}

} // namespace basicleds
