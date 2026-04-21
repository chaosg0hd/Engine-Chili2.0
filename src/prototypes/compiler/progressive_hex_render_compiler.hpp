#pragma once

#include "progressive_hex_render_strategy.hpp"

#include "../presentation/frame.hpp"

enum class ProgressiveHexRenderPresentationModePrototype : unsigned char
{
    NativeHexPatches = 0,
    RegionRectangles
};

struct ProgressiveHexRenderPresentationOptionsPrototype
{
    ProgressiveHexRenderPresentationModePrototype mode =
        ProgressiveHexRenderPresentationModePrototype::NativeHexPatches;
    unsigned int maxRegionPatches = 9000U;
};

struct ProgressiveHexRenderPresentationResultPrototype
{
    FramePrototype frame;
    unsigned int renderedRegionPatchCount = 0U;
};

class ProgressiveHexRenderCompiler
{
public:
    static ProgressiveHexRenderPresentationResultPrototype Compile(
        const ProgressiveHexRenderStrategyPrototype& strategy,
        const ProgressiveHexRenderPresentationOptionsPrototype& options);
};
