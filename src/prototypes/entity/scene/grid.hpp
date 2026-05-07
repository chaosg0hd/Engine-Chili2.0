#pragma once

#include "../../math/math.hpp"

#include <cstdint>

struct GridPrototype
{
    Vector3 origin = Vector3(0.0f, 0.0f, 0.0f);
    float extent = 24.0f;
    float cellSize = 1.0f;
    int majorLineEvery = 4;
    std::uint32_t baseColor = 0xFF141A22u;
    std::uint32_t majorLineColor = 0xFF3A4A60u;
    std::uint32_t minorLineColor = 0xFF263446u;
    float lineThickness = 0.018f;
};
