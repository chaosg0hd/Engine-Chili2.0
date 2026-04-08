#pragma once

#include "render_material.hpp"
#include "render_math.hpp"
#include "render_mesh.hpp"

struct RenderObjectPrototype
{
    RenderTransform transform;
    RenderMeshPrototype mesh;
    RenderMaterialPrototype material;
};
