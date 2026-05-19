#pragma once

#include "../entity/appearance/color.hpp"
#include "../entity/geometry/line.hpp"
#include "../entity/object/object.hpp"
#include "../entity/scene/grid.hpp"
#include "../entity/scene/infinite_plane.hpp"

enum class ItemKind : unsigned char
{
    Unknown = 0,
    Object3D,
    Line,
    Grid,
    InfinitePlane,
    Overlay2D,
    ScreenPatch,
    ScreenHexPatch
};

enum class LineRenderStyle : unsigned char
{
    Solid = 0,
    Broken
};

struct LineItemPrototype
{
    LinePrototype geometry;
    ColorPrototype color;
    float thickness = 0.02f;
    float fallbackLength = 1.0f;
    LineRenderStyle style = LineRenderStyle::Solid;
    float brokenDashLength = 0.35f;
    float brokenGapLength = 0.2f;
};

struct ScreenPatchPrototype
{
    float centerX = 0.0f;
    float centerY = 0.0f;
    float halfWidth = 0.1f;
    float halfHeight = 0.1f;
    float rotationRadians = 0.0f;
    std::uint32_t color = 0xFFFFFFFFu;
};

struct ItemPrototype
{
    ItemKind kind = ItemKind::Unknown;
    ObjectPrototype object3D;
    LineItemPrototype line;
    GridPrototype grid;
    InfinitePlanePrototype infinitePlane;
    ScreenPatchPrototype screenPatch;
};
