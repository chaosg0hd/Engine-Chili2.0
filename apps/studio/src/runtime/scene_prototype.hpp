#pragma once

#include "prototypes/entity/appearance/material.hpp"
#include "prototypes/entity/object/mesh.hpp"
#include "prototypes/math/math.hpp"

#include <string>
#include <vector>

namespace studio_runtime
{
    struct EntityPrototype
    {
        Vector3 position = Vector3(0.0f, 0.0f, 0.0f);
        BuiltInMeshKind mesh = BuiltInMeshKind::Cube;
        std::string materialRef = "default";
    };

    struct ScenePrototype
    {
        std::vector<EntityPrototype> entities;
    };
}

