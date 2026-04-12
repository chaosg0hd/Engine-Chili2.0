#pragma once

#include "color.hpp"

#include <algorithm>

struct AbsorptionPrototype
{
    ColorPrototype absorptionColor = ColorPrototype(0.0f, 0.0f, 0.0f, 1.0f);
    float absorption = 0.0f;

    bool IsAbsorbing() const
    {
        return absorption > 0.0f;
    }

    float ApplyAbsorptionToIntensity(float incomingIntensity) const
    {
        return incomingIntensity * (1.0f - std::clamp(absorption, 0.0f, 1.0f));
    }
};
