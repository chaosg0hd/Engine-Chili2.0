#pragma once

#include "../entity/object/object.hpp"
#include "../entity/scene/infinite_plane.hpp"

enum class ItemKind : unsigned char
{
    Unknown = 0,
    Object3D,
    InfinitePlane,
    Overlay2D
};

struct ItemPrototype
{
    ItemKind kind = ItemKind::Unknown;
    ObjectPrototype object3D;
    InfinitePlanePrototype infinitePlane;
};
