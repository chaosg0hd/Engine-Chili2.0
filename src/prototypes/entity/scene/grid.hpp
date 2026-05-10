#pragma once

#include "../../math/math.hpp"
#include "../appearance/color.hpp"

struct GridPrototype
{
    Vector3 origin = Vector3(0.0f, 0.0f, 0.0f);
    float extent = 24.0f;
    float cellSize = 1.0f;
    int majorLineEvery = 4;
    ColorPrototype baseColor = ColorPrototype::FromArgb(0xFF141A22u);
    ColorPrototype majorLineColor = ColorPrototype::FromArgb(0xFF3A4A60u);
    ColorPrototype minorLineColor = ColorPrototype::FromArgb(0xFF263446u);
    float lineThickness = 0.018f;
};
