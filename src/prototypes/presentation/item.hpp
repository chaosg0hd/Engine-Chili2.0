#pragma once

#include "../entity/object/object.hpp"
#include "../entity/scene/infinite_plane.hpp"

#include <cstdint>

enum class ItemKind : unsigned char
{
    Unknown = 0,
    Object3D,
    InfinitePlane,
    Overlay2D,
    ScreenCell
};

struct ScreenCellPrototype
{
    float centerX = 0.0f;
    float centerY = 0.0f;
    float halfWidth = 0.1f;
    float halfHeight = 0.1f;
    std::uint32_t color = 0xFFFFFFFFu;
};

struct ItemPrototype
{
    ItemKind kind = ItemKind::Unknown;
    ObjectPrototype object3D;
    InfinitePlanePrototype infinitePlane;
    ScreenCellPrototype screenCell;
};
