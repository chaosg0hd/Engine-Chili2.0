#pragma once

#include "render_camera.hpp"
#include "render_item.hpp"

#include <vector>

enum class RenderViewKind : unsigned char
{
    Unknown = 0,
    Scene3D,
    Overlay2D
};

struct RenderViewPrototype
{
    RenderViewKind kind = RenderViewKind::Unknown;
    RenderCamera camera;
    std::vector<RenderItemPrototype> items;
};
