#pragma once

#include "../../modules/render/render_frame_data.hpp"
#include "../presentation/frame.hpp"

class RenderFrameCompiler
{
public:
    static RenderFrameData Compile(const FramePrototype& frame);
};
