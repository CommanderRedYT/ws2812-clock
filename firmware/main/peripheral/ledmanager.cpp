#include "ledmanager.h"

constexpr const char * const TAG = "ledmanager";

// 3rdparty lib includes
#include <FastLED.h>
#include <espchrono.h>
#include <fmt/core.h>
#include <recursivelockhelper.h>

// local includes
#include "communication/ota.h"
#include "peripheral/ledhelpers/ledanimation.h"
#include "utils/config.h"
#include "utils/espclock.h"

using namespace std::chrono_literals;

namespace ledmanager {

cpputils::DelayedConstruction<LedManager> ledManager;

cpputils::DelayedConstruction<espcpputils::recursive_mutex_semaphore> led_lock;

namespace {
LedArray leds;

bool calculateLedVisibility()
{
    if (configs.showUnsyncedTime.value())
    {
        return true;
    }

    if (espclock::isSynced())
    {
        return true;
    }

    return false;
}

} // namespace

void LedManager::handleVoltageAndCurrent()
{
    const auto now = espchrono::millis_clock::now();

    FastLED.setBrightness(m_brightness);

    uint8_t brightnessTarget = m_visible ? configs.ledBrightness.value() : 0;

    if (espchrono::ago(m_brightnessLastUpdate) > 25ms)
    {
        if (m_brightness < brightnessTarget)
        {
            m_brightness++;
        }
        else if (m_brightness > brightnessTarget)
        {
            m_brightness--;
        }

        m_brightnessLastUpdate = now;
    }

    if (/*barrel_jack_connected*/false)
    {
        // increase current
        // 5V, 8A
        FastLED.setMaxPowerInVoltsAndMilliamps(5, 8000);
    }
    else
    {
        // 5V, 3A
        FastLED.setMaxPowerInVoltsAndMilliamps(5, 3000);
    }
}

void begin()
{
    led_lock.construct();

    FastLED.addLeds<WS2812B, HARDWARE_WS2812B_PIN, HARDWARE_WS2812B_COLOR_ORDER>(leds.data(), leds.size());

    FastLED.setCorrection(TypicalSMD5050);

    FastLED.setDither(0);

    FastLED.clear();

    // Digit 1: LED_1 -> LED_56 => start 0, length 56
    // Digit 2: LED_57 -> LED_112 => start 56, length 56
    // Upper Dot: LED_113 -> LED_116 => start 112, length 4
    // Lower Dot: LED_117 -> LED_120 => start 116, length 4
    // Digit 3: LED_121 -> LED_176 => start 120, length 56
    // Digit 4: LED_177 -> LED_232 => start 176, length 56

    ledManager.construct(LedManager{
        {
            SevenSegmentDigit{leds.data() + 0, 56},
            SevenSegmentDigit{leds.data() + 56, 56},
            SevenSegmentDigit{leds.data() + 120, 56},
            SevenSegmentDigit{leds.data() + 176, 56},
        },
        ClockDot{ClockDot::Top, leds.data() + 112, 4},
        ClockDot{ClockDot::Bottom, leds.data() + 116, 4},
    });

    for (auto& digit : ledManager->digits)
    {
        digit.setChar('1');
    }
}

void update()
{
    espcpputils::RecursiveLockHelper guard{led_lock->handle};

    if (ota::isInProgress()) // Prevent guru meditation error because of rmt inline not in IRAM
        return;

    if (ledManager.constructed())
    {
        ledManager->setVisible(calculateLedVisibility());

        ledManager->render();

        ledManager->handleVoltageAndCurrent();

        FastLED.show();
    }
}

void LedManager::render()
{
    const auto now = espchrono::millis_clock::now();

    const bool dotsOn = configs.disableDotBlinking.value() || now.time_since_epoch() % 1s < 500ms;

    upper.on(dotsOn);
    lower.on(dotsOn);

    if (const auto res = animation::updateAnimation(configs.ledAnimation.value().c_str(), leds); !res)
    {
        ESP_LOGE(TAG, "Failed to update animation: %.*s\n", res.error().size(), res.error().data());
    }
    else if (auto animation = animation::currentAnimation; animation)
    {
        if (animation->needsUpdate())
        {
            for (auto &digit: digits)
            {
                animation->update();
                if (animation->rendersOnce())
                {
                    animation->render_all(leds.begin(), leds.size());
                }
                digit.forEverySegment([&](SevenSegmentDigit::Segment segment, CRGB *startLed, CRGB *endLed) {
                    animation->render_segment(segment, digit, startLed, digits.size());
                });
            }

            animation->render_dot(upper, leds.begin(), leds.size());
            animation->render_dot(lower, leds.begin(), leds.size());
        }
    }
    else
    {
        upper.render();
        lower.render();
    }

    if (!configs.noClockDigits.value())
    {
        for (auto &digit: digits)
        {
            digit.renderMask();
        }
    }
}

std::string LedManager::toString() const
{
    return fmt::format("LedManager digit0={} digit1={} digit2={} digit3={} upper={} lower={}",
                       digits[0].toString(),
                       digits[1].toString(),
                       digits[2].toString(),
                       digits[3].toString(),
                       upper.toString(),
                       lower.toString());
}

uint16_t LedManager::getFps()
{
    return FastLED.getFPS();
}

const std::array<CRGB, HARDWARE_WS2812B_COUNT>& getLeds()
{
    return leds;
}

} // namespace ledmanager
