#pragma once

#include "../../modules/render/render_frame_data.hpp"
#include "../entity/object/object.hpp"

#include <vector>

class ObjectRenderCompiler
{
public:
    static void Append(const ObjectPrototype& object, std::vector<RenderItemData>& outItems);
};
