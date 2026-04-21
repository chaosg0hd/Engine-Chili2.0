#include "progressive_hex_render_compiler.hpp"

#include "../presentation/item.hpp"
#include "../presentation/pass.hpp"
#include "../presentation/view.hpp"

#include <algorithm>
#include <vector>

namespace
{
float ClampScreenCoordinate(float value)
{
    return std::max(-1.0f, std::min(1.0f, value));
}

void AppendProgressiveHexPatches(
    ViewPrototype& view,
    const ProgressiveHexRenderStrategyPrototype& strategy)
{
    std::vector<ProgressiveHexRenderPatchPrototype> patches;
    strategy.BuildPatches(patches);
    view.items.reserve(view.items.size() + patches.size());

    for (const ProgressiveHexRenderPatchPrototype& patch : patches)
    {
        ItemPrototype item;
        item.kind = patch.useHexShape ? ItemKind::ScreenHexPatch : ItemKind::ScreenPatch;
        item.screenPatch.centerX = patch.centerX;
        item.screenPatch.centerY = patch.centerY;
        item.screenPatch.halfWidth = patch.halfWidth;
        item.screenPatch.halfHeight = patch.halfHeight;
        item.screenPatch.rotationRadians = patch.rotationRadians;
        item.screenPatch.color = patch.color;
        view.items.push_back(item);
    }
}

unsigned int AppendRegionRectanglePatches(
    ViewPrototype& view,
    const ProgressiveHexRenderStrategyPrototype& strategy,
    unsigned int maxRegionPatches)
{
    std::vector<ProgressiveHexRenderRegionUpdatePrototype> regionJobs;
    strategy.BuildRegionUpdateJobs(regionJobs);

    const ProgressiveHexRenderStatsPrototype& stats = strategy.GetStats();
    const unsigned int maxDepth = stats.maxSubdivisionDepth;
    std::vector<const ProgressiveHexRenderRegionUpdatePrototype*> renderJobs;
    renderJobs.reserve(regionJobs.size());
    for (const ProgressiveHexRenderRegionUpdatePrototype& job : regionJobs)
    {
        if (job.depth == maxDepth && job.coveredCellCount > 0U && job.resolvedCellCount > 0U)
        {
            renderJobs.push_back(&job);
        }
    }

    if (renderJobs.empty())
    {
        AppendProgressiveHexPatches(view, strategy);
        return 0U;
    }

    const unsigned int targetPatchCount = std::min(
        maxRegionPatches,
        static_cast<unsigned int>(renderJobs.size()));
    const unsigned int stride = std::max(
        1U,
        static_cast<unsigned int>((renderJobs.size() + targetPatchCount - 1U) / targetPatchCount));

    view.items.reserve(view.items.size() + targetPatchCount);
    unsigned int emittedCount = 0U;
    for (std::size_t index = 0U; index < renderJobs.size() && emittedCount < targetPatchCount; index += stride)
    {
        const ProgressiveHexRenderRegionUpdatePrototype& job = *renderJobs[index];
        const float minX = ClampScreenCoordinate(job.minX);
        const float minY = ClampScreenCoordinate(job.minY);
        const float maxX = ClampScreenCoordinate(job.maxX);
        const float maxY = ClampScreenCoordinate(job.maxY);
        const float halfWidth = std::max(0.00075f, (maxX - minX) * 0.5f);
        const float halfHeight = std::max(0.00075f, (maxY - minY) * 0.5f);

        ItemPrototype item;
        item.kind = ItemKind::ScreenPatch;
        item.screenPatch.centerX = (minX + maxX) * 0.5f;
        item.screenPatch.centerY = (minY + maxY) * 0.5f;
        item.screenPatch.halfWidth = halfWidth;
        item.screenPatch.halfHeight = halfHeight;
        item.screenPatch.rotationRadians = 0.0f;
        item.screenPatch.color = job.color;
        view.items.push_back(item);
        ++emittedCount;
    }

    return emittedCount;
}
}

ProgressiveHexRenderPresentationResultPrototype ProgressiveHexRenderCompiler::Compile(
    const ProgressiveHexRenderStrategyPrototype& strategy,
    const ProgressiveHexRenderPresentationOptionsPrototype& options)
{
    ProgressiveHexRenderPresentationResultPrototype result;

    ViewPrototype view;
    view.kind = ViewKind::Overlay2D;

    switch (options.mode)
    {
    case ProgressiveHexRenderPresentationModePrototype::RegionRectangles:
        result.renderedRegionPatchCount = AppendRegionRectanglePatches(
            view,
            strategy,
            options.maxRegionPatches);
        break;
    case ProgressiveHexRenderPresentationModePrototype::NativeHexPatches:
    default:
        AppendProgressiveHexPatches(view, strategy);
        result.renderedRegionPatchCount = 0U;
        break;
    }

    PassPrototype pass;
    pass.kind = PassKind::Overlay;
    pass.views.push_back(std::move(view));
    result.frame.passes.push_back(std::move(pass));
    return result;
}
