#include "progressive_hex_render_controller.hpp"

#include <algorithm>
#include <utility>
#include <vector>

namespace render
{
void ProgressiveHexRenderController::Configure(const ProgressiveHexRenderControllerOptions& options)
{
    m_options = options;
    m_strategy.Configure(m_options.strategyConfig);
}

void ProgressiveHexRenderController::SetSampleProvider(ProgressiveHexSampleProvider sampleProvider)
{
    m_strategy.SetSampleProvider(std::move(sampleProvider));
}

void ProgressiveHexRenderController::Reset()
{
    m_strategy.Reset();
    m_adaptiveRenderBudgetSeconds = 0.002;
    m_renderedRegionPatchCount = 0U;
    m_algorithmPreviewStep = 0U;
    m_algorithmPreviewProgressCount = 0U;
    m_maxDepthReached = false;
}

void ProgressiveHexRenderController::Update(
    const ProgressiveHexRenderControllerContext& context,
    const ProgressiveHexJobSubmitter& submitJob,
    const ProgressiveHexJobWaiter& waitForJobs)
{
    m_strategy.Resize(context.screenWidth, context.screenHeight, context.aspectRatio);

    m_adaptiveRenderBudgetSeconds = ComputeAdaptiveRenderBudget(context);
    m_strategy.Step(
        context.totalTime,
        context.deltaTime,
        m_adaptiveRenderBudgetSeconds,
        std::max(1U, context.idleWorkerCount),
        submitJob,
        waitForJobs);

    const ProgressiveHexRenderStatsPrototype& stats = m_strategy.GetStats();
    m_strategy.SetAlgorithmPreviewCycle(0U);
    m_algorithmPreviewStep = stats.centerPassCount > 0U ? 1U : 0U;
    m_algorithmPreviewProgressCount = stats.centerPassCount;
    m_strategy.SetAlgorithmPreviewStep(
        m_algorithmPreviewStep,
        m_algorithmPreviewProgressCount);
    m_maxDepthReached =
        m_algorithmPreviewStep == 1U &&
        stats.centerPassCount > 0U &&
        stats.visibleCenterPassCount >= stats.centerPassCount;
}

FramePrototype ProgressiveHexRenderController::BuildFrame()
{
    ProgressiveHexRenderPresentationResultPrototype result =
        ProgressiveHexRenderCompiler::Compile(m_strategy, m_options.presentationOptions);
    m_renderedRegionPatchCount = result.renderedRegionPatchCount;
    return result.frame;
}

std::wstring ProgressiveHexRenderController::BuildOverlayText() const
{
    const ProgressiveHexRenderStatsPrototype& stats = m_strategy.GetStats();
    const ProgressiveHexRenderConfigPrototype& config = m_strategy.GetConfig();
    std::vector<ProgressiveHexRenderRegionUpdatePrototype> regionJobs;
    m_strategy.BuildRegionUpdateJobs(regionJobs);

    unsigned int deepestJobDepth = 0U;
    unsigned int deepestJobCount = 0U;
    for (const ProgressiveHexRenderRegionUpdatePrototype& job : regionJobs)
    {
        if (job.depth > deepestJobDepth)
        {
            deepestJobDepth = job.depth;
            deepestJobCount = 1U;
        }
        else if (job.depth == deepestJobDepth)
        {
            ++deepestJobCount;
        }
    }

    return
        m_options.overlayTitle + L"\n"
        L"  Screen: " + std::to_wstring(stats.screenWidth) + L" x " + std::to_wstring(stats.screenHeight) + L"\n"
        L"  Hex Size: " + std::to_wstring(config.hexSize) + L"\n"
        L"  Candidate Radius: " + std::to_wstring(stats.candidateRadius) + L"\n"
        L"  Macro Hex Radius: " + std::to_wstring(stats.majorCenterRegionRadius) + L"\n"
        L"  Macro Region Count: " + std::to_wstring(stats.regionCount) + L"\n"
        L"  Grid Cells: " + std::to_wstring(stats.generatedHexCount) + L"\n"
        L"  Rejected Edge Cells: " + std::to_wstring(stats.rejectedHexCount) + L"\n"
        L"  Algo 0 Step: " + BuildAlgorithmPreviewStepText() + L"\n"
        L"  Runtime List: " + std::to_wstring(stats.visibleCenterPassCount) + L" / " + std::to_wstring(stats.centerPassCount) + L" centers\n"
        L"  Center Passes: " + std::to_wstring(stats.visibleCenterPassCount) + L" / " + std::to_wstring(stats.centerPassCount) + L"\n"
        L"  Weighted Runtime Slots: " + std::to_wstring(stats.fillStepCount) + L" (weighted phase, n^2 + center boost)\n"
        L"  Render Region Jobs: " + std::to_wstring(regionJobs.size()) + L" / deepest " + std::to_wstring(deepestJobDepth) + L" x " + std::to_wstring(deepestJobCount) + L"\n"
        L"  Renderer Patch Test: " + std::wstring(
            m_options.presentationOptions.mode == ProgressiveHexRenderPresentationModePrototype::RegionRectangles
                ? L"algo resolved region rectangles"
                : L"native progressive hex patches") + L"\n"
        L"  Rendered Region Patches: " + std::to_wstring(m_renderedRegionPatchCount) + L"\n"
        L"  Runtime Cycle: " + std::wstring(m_maxDepthReached ? L"complete - looping mapped order" : L"running") + L"\n"
        L"  Budget: protected full fill, " + std::to_wstring(m_adaptiveRenderBudgetSeconds) + L" sec\n"
        L"  Workers: " + std::to_wstring(stats.lastParallelWorkerCount) + L" idle-fed\n"
        L"  Fill Order: enabled\n"
        L"  Sample: " + m_options.sampleLabel + L"\n"
        L"  Display Colors: resolved scene color\n"
        L"  Pass Age Fade: " + std::wstring(config.fadeByTimeSincePass ? L"on" : L"off") + L"\n"
        L"  R: regenerate\n"
        L"  Escape: exit\n";
}

std::string ProgressiveHexRenderController::BuildTraversalLog() const
{
    return m_strategy.BuildAlgorithmPreviewTraversalLog();
}

std::string ProgressiveHexRenderController::BuildRuntimeOrderLog() const
{
    return m_strategy.BuildRuntimePassOrderLog();
}

std::string ProgressiveHexRenderController::BuildCenterPassLog() const
{
    return m_strategy.BuildAlgorithmMasterListLog();
}

ProgressiveHexRenderController::DebugLogBundle ProgressiveHexRenderController::BuildDebugLogBundle() const
{
    DebugLogBundle bundle;
    bundle.traversalLog = BuildTraversalLog();
    bundle.runtimeOrderLog = BuildRuntimeOrderLog();
    bundle.centerPassLog = BuildCenterPassLog();
    return bundle;
}

const ProgressiveHexRenderStrategyPrototype& ProgressiveHexRenderController::GetStrategy() const
{
    return m_strategy;
}

const ProgressiveHexRenderControllerOptions& ProgressiveHexRenderController::GetOptions() const
{
    return m_options;
}

double ProgressiveHexRenderController::GetAdaptiveRenderBudgetSeconds() const
{
    return m_adaptiveRenderBudgetSeconds;
}

unsigned int ProgressiveHexRenderController::GetRenderedRegionPatchCount() const
{
    return m_renderedRegionPatchCount;
}

unsigned int ProgressiveHexRenderController::GetAlgorithmPreviewStep() const
{
    return m_algorithmPreviewStep;
}

unsigned int ProgressiveHexRenderController::GetAlgorithmPreviewProgressCount() const
{
    return m_algorithmPreviewProgressCount;
}

bool ProgressiveHexRenderController::HasReachedMaxDepth() const
{
    return m_maxDepthReached;
}

double ProgressiveHexRenderController::ComputeAdaptiveRenderBudget(
    const ProgressiveHexRenderControllerContext& context)
{
    constexpr double MinimumBudgetSeconds = 0.00025;
    constexpr double ConservativeBudgetSeconds = 0.00100;
    constexpr double MaximumBudgetSeconds = 0.00600;

    if (context.behindSchedule)
    {
        return MinimumBudgetSeconds;
    }

    if (context.lastFrameWaitDuration > 0.0)
    {
        return std::max(
            ConservativeBudgetSeconds,
            std::min(MaximumBudgetSeconds, context.lastFrameWaitDuration * 0.75));
    }

    if (context.targetFrameTime > 0.0 && context.lastFrameWorkDuration > 0.0)
    {
        const double estimatedSlack = context.targetFrameTime - context.lastFrameWorkDuration;
        if (estimatedSlack > 0.0)
        {
            return std::max(
                ConservativeBudgetSeconds,
                std::min(MaximumBudgetSeconds, estimatedSlack * 0.50));
        }
    }

    return ConservativeBudgetSeconds;
}

std::wstring ProgressiveHexRenderController::BuildAlgorithmPreviewStepText() const
{
    switch (m_algorithmPreviewStep)
    {
    case 0U:
        return L"setup: subdivide and map cells";
    case 1U:
        return L"runtime: walk center-pass list";
    default:
        return L"unknown";
    }
}
}
