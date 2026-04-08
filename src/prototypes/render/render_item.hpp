#pragma once

#include "render_object.hpp"

enum class RenderItemKind : unsigned char
{
    Unknown = 0,
    Object3D,
    Overlay2D
};

struct RenderItemPrototype
{
    RenderItemKind kind = RenderItemKind::Unknown;
    RenderObjectPrototype object3D;
};
