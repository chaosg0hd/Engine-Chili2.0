#pragma once

#include <cstdint>

using RenderMaterialHandle = std::uint32_t;

struct RenderMaterialPrototype
{
    RenderMaterialHandle handle = 0;
    std::uint32_t baseColor = 0xFFFFFFFFu;
};
