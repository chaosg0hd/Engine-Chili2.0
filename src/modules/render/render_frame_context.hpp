#pragma once

#include "render_types.hpp"

#include <cstdint>

struct RenderFrameContext
{
    double deltaTime = 0.0;
    std::uint64_t frameIndex = 0;
    RenderViewport viewport;
    RenderClearColor clearColor;
};
