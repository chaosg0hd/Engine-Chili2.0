#pragma once

#include "absorption.hpp"
#include "color.hpp"
#include "reflection.hpp"

#include <cstdint>

using MaterialHandle = std::uint32_t;

struct MaterialPrototype : public ReflectionPrototype, public AbsorptionPrototype
{
    MaterialHandle handle = 0;
    ColorPrototype baseColor;
};
