#pragma once

#include "color.hpp"

struct ReflectionPrototype
{
    ColorPrototype reflectionColor = ColorPrototype(1.0f, 1.0f, 1.0f, 1.0f);
    float reflectivity = 0.0f;
    float roughness = 1.0f;

    bool IsReflective() const
    {
        return reflectivity > 0.0f;
    }
};
