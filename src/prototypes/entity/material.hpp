#pragma once

#include <cstdint>

using MaterialHandle = std::uint32_t;

struct MaterialPrototype
{
    MaterialHandle handle = 0;
    std::uint32_t baseColor = 0xFFFFFFFFu;
};
