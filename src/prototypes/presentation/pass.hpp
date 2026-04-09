#pragma once

#include "view.hpp"

#include <vector>

enum class PassKind : unsigned char
{
    Unknown = 0,
    Scene,
    Overlay,
    Composite
};

struct PassPrototype
{
    PassKind kind = PassKind::Unknown;
    std::vector<ViewPrototype> views;
};
