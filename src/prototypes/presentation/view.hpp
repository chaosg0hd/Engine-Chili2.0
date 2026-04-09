#pragma once

#include "../entity/camera.hpp"
#include "item.hpp"

#include <vector>

enum class ViewKind : unsigned char
{
    Unknown = 0,
    Scene3D,
    Overlay2D
};

struct ViewPrototype
{
    ViewKind kind = ViewKind::Unknown;
    CameraPrototype camera;
    std::vector<ItemPrototype> items;
};
