#pragma once

#include "../entity/scene/camera.hpp"
#include "../entity/scene/grid.hpp"

#include "../../modules/render/render_frame_data.hpp"

#include <vector>

class GridRenderCompiler
{
public:
    static void Append(
        const GridPrototype& grid,
        const CameraPrototype& camera,
        std::vector<RenderItemData>& outItems);
};
