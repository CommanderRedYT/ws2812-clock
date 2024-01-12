#include "ledanimation.h"

constexpr const char * const TAG = "ledanimation";

// 3rdparty lib includes
#include "arrayview.h"

// local includes
#include "communication/ota.h"
#include "peripheral/ledhelpers/animations/internal/otaanimation.h"
#include "utils/config.h"

// animations
#include "peripheral/ledhelpers/animations/rainbowanimation.h"
#include "peripheral/ledhelpers/animations/staticcoloranimation.h"
#include "peripheral/ledhelpers/animations/newyearanimation.h"

namespace animation {

LedAnimation* animationsArr[]{
    new RainbowAnimation(),
    new StaticColorAnimation(),
    new NewYearAnimation(),
};

cpputils::ArrayView<LedAnimation*> animations{animationsArr};

LedAnimation* currentAnimation{nullptr};

const LedAnimation& getFirstAnimation()
{
    return *animations[0];
}

std::expected<void, std::string> updateAnimation(LedAnimationName enumValue, ledmanager::LedArray& leds)
{
    LedAnimation* newAnimation{nullptr};

    if (currentAnimation != nullptr && currentAnimation->getEnumValue() == enumValue && !ota::isInProgress())
        return {};

    if (ota::isInProgress())
    {
        newAnimation = internal::otaAnimation;
    }
    else
    {
        for (auto animation: animations)
        {
            if (animation->getEnumValue() == enumValue)
            {
                newAnimation = animation;
                break;
            }
        }
    }


    if (newAnimation != nullptr)
    {
        if (currentAnimation)
        {
            // void stop(CRGB* startLed, CRGB* endLed)
            currentAnimation->stop(leds.data(), leds.size());
        }
        currentAnimation = newAnimation;
        currentAnimation->start(leds.data(), leds.size());
        return {};
    }

    return std::unexpected(fmt::format("Animation '{}' not found ({})", toString(enumValue), std::to_underlying(enumValue)));
}

bool animationExists(LedAnimationName enumValue)
{
    return std::ranges::any_of(animations, [enumValue](const LedAnimation* animation) {
        return animation->getEnumValue() == enumValue;
    });
}

bool LedAnimation::needsUpdate() const
{
    if (!m_lastUpdate.has_value())
    {
        return true;
    }

    // return espchrono::ago(*m_lastUpdate) > getUpdateInterval() / m_speedMultiplier;
    return espchrono::ago(*m_lastUpdate) > getUpdateInterval() / configs.animationMultiplier.value();
}
} // namespace animation
