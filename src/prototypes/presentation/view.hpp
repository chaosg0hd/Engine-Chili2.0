#pragma once

#include "../entity/appearance/light.hpp"
#include "../entity/scene/camera.hpp"
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
    std::vector<LightPrototype> lights;
    std::vector<ItemPrototype> items;
};
