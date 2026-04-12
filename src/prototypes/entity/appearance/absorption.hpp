#pragma once

#include "color.hpp"

struct AbsorptionPrototype
{
    ColorPrototype absorptionColor = ColorPrototype(0.0f, 0.0f, 0.0f, 1.0f);
    float absorption = 0.0f;

    bool IsAbsorbing() const
    {
        return absorption > 0.0f;
    }
};
