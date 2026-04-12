#pragma once

#include "../entity/scene/camera.hpp"
#include "../systems/light_ray.hpp"
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
    std::vector<LightRayEmitterPrototype> lightRayEmitters;
    std::vector<ItemPrototype> items;
};
