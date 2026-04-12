#pragma once

#include "../../math/math.hpp"

#include <cstdint>

struct InfinitePlanePrototype
{
    Vector3 origin = Vector3(0.0f, -2.0f, 0.0f);
    Vector3 normal = Vector3(0.0f, 1.0f, 0.0f);
    std::uint32_t baseColor = 0xFF18222Eu;
    std::uint32_t minorLineColor = 0xFF273647u;
    std::uint32_t majorLineColor = 0xFF4D6A84u;
    float majorGridSpacing = 5.0f;
    float minorGridSpacing = 1.0f;
    float lineThickness = 0.035f;
    float extent = 20.0f;
};
