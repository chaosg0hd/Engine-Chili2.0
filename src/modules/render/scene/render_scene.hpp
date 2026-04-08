#pragma once

#include "render_camera.hpp"
#include "render_item.hpp"

#include <vector>

struct RenderScene
{
    RenderCamera camera;
    std::vector<RenderItem> items;
};
