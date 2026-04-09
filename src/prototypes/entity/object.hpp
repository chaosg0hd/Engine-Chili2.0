#pragma once

#include "material.hpp"
#include "../math/math.hpp"
#include "mesh.hpp"

struct ObjectPrototype
{
    TransformPrototype transform;
    MeshPrototype mesh;
    MaterialPrototype material;
};
