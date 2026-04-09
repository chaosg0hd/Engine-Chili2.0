#pragma once

#include "../entity/object.hpp"

enum class ItemKind : unsigned char
{
    Unknown = 0,
    Object3D,
    Overlay2D
};

struct ItemPrototype
{
    ItemKind kind = ItemKind::Unknown;
    ObjectPrototype object3D;
};
