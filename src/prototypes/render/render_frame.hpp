#pragma once

#include "render_pass.hpp"

#include <vector>

struct RenderFramePrototype
{
    std::vector<RenderPassPrototype> passes;
};
