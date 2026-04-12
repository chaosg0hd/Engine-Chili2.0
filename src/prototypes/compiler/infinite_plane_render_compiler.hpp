#pragma once

#include "../../modules/render/render_frame_data.hpp"
#include "../entity/scene/camera.hpp"
#include "../entity/scene/infinite_plane.hpp"

#include <vector>

class InfinitePlaneRenderCompiler
{
public:
    static void Append(
        const InfinitePlanePrototype& plane,
        const CameraPrototype& camera,
        std::vector<RenderItemData>& outItems);
};
