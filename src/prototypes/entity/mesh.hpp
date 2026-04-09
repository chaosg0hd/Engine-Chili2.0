#pragma once

#include <cstdint>

using MeshHandle = std::uint32_t;

enum class BuiltInMeshKind : unsigned char
{
    None = 0,
    Triangle,
    Diamond,
    Cube,
    Quad,
    Octahedron
};

struct MeshPrototype
{
    MeshHandle handle = 0;
    BuiltInMeshKind builtInKind = BuiltInMeshKind::None;
};
