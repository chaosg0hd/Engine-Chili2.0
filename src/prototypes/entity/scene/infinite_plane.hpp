#pragma once

#include "../../math/math.hpp"
#include "../appearance/color.hpp"

struct InfinitePlanePrototype
{
    Vector3 origin = Vector3(0.0f, -2.0f, 0.0f);
    Vector3 normal = Vector3(0.0f, 1.0f, 0.0f);
    ColorPrototype baseColor = ColorPrototype::FromArgb(0xFF18222Eu);
    ColorPrototype minorLineColor = ColorPrototype::FromArgb(0xFF273647u);
    ColorPrototype majorLineColor = ColorPrototype::FromArgb(0xFF4D6A84u);
    float majorGridSpacing = 5.0f;
    float minorGridSpacing = 1.0f;
    float lineThickness = 0.035f;
    float extent = 20.0f;
};
