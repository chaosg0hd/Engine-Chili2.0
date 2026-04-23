#pragma once

#include "color.hpp"

struct IndirectLightProbePrototype
{
    Vector3 position = Vector3(0.0f, 0.0f, 0.0f);
    ColorPrototype indirectColor = ColorPrototype(1.0f, 1.0f, 1.0f, 1.0f);
    float intensity = 0.0f;
    float radius = 1.0f;
    bool enabled = true;
};
