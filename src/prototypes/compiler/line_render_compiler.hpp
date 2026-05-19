#pragma once

#include "../../modules/render/render_frame_data.hpp"
#include "../entity/appearance/color.hpp"
#include "../entity/geometry/line.hpp"
#include "../presentation/item.hpp"

#include <vector>

class LineRenderCompiler
{
public:
    static void Append(
        const LinePrototype& line,
        const ColorPrototype& color,
        float thickness,
        float fallbackLength,
        LineRenderStyle style,
        float brokenDashLength,
        float brokenGapLength,
        std::vector<RenderItemData>& outItems);
};
