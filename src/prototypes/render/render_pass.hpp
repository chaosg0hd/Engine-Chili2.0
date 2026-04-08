#pragma once

#include "render_view.hpp"

#include <vector>

enum class RenderPassKind : unsigned char
{
    Unknown = 0,
    Scene,
    Overlay,
    Composite
};

struct RenderPassPrototype
{
    RenderPassKind kind = RenderPassKind::Unknown;
    std::vector<RenderViewPrototype> views;
};
