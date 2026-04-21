#pragma once

#include "../../prototypes/compiler/progressive_hex_render_compiler.hpp"

#include <string>

namespace render
{
struct ProgressiveHexRenderControllerContext
{
    int screenWidth = 0;
    int screenHeight = 0;
    float aspectRatio = 1.0f;
    double totalTime = 0.0;
    double deltaTime = 0.0;
    bool behindSchedule = false;
    double lastFrameWaitDuration = 0.0;
    double targetFrameTime = 0.0;
    double lastFrameWorkDuration = 0.0;
    unsigned int idleWorkerCount = 1U;
};

struct ProgressiveHexRenderControllerOptions
{
    ProgressiveHexRenderConfigPrototype strategyConfig;
    ProgressiveHexRenderPresentationOptionsPrototype presentationOptions;
    std::wstring overlayTitle = L"PROGRESSIVE HEX";
    std::wstring sampleLabel = L"scene";
};

class ProgressiveHexRenderController
{
public:
    struct DebugLogBundle
    {
        std::string traversalLog;
        std::string runtimeOrderLog;
        std::string centerPassLog;
    };

    void Configure(const ProgressiveHexRenderControllerOptions& options);
    void SetSampleProvider(ProgressiveHexSampleProvider sampleProvider);
    void Reset();
    void Update(
        const ProgressiveHexRenderControllerContext& context,
        const ProgressiveHexJobSubmitter& submitJob = ProgressiveHexJobSubmitter{},
        const ProgressiveHexJobWaiter& waitForJobs = ProgressiveHexJobWaiter{});

    FramePrototype BuildFrame();
    std::wstring BuildOverlayText() const;

    std::string BuildTraversalLog() const;
    std::string BuildRuntimeOrderLog() const;
    std::string BuildCenterPassLog() const;
    DebugLogBundle BuildDebugLogBundle() const;

    const ProgressiveHexRenderStrategyPrototype& GetStrategy() const;
    const ProgressiveHexRenderControllerOptions& GetOptions() const;
    double GetAdaptiveRenderBudgetSeconds() const;
    unsigned int GetRenderedRegionPatchCount() const;
    unsigned int GetAlgorithmPreviewStep() const;
    unsigned int GetAlgorithmPreviewProgressCount() const;
    bool HasReachedMaxDepth() const;

private:
    static double ComputeAdaptiveRenderBudget(const ProgressiveHexRenderControllerContext& context);
    std::wstring BuildAlgorithmPreviewStepText() const;

private:
    ProgressiveHexRenderControllerOptions m_options;
    ProgressiveHexRenderStrategyPrototype m_strategy;
    double m_adaptiveRenderBudgetSeconds = 0.002;
    unsigned int m_renderedRegionPatchCount = 0U;
    unsigned int m_algorithmPreviewStep = 0U;
    unsigned int m_algorithmPreviewProgressCount = 0U;
    bool m_maxDepthReached = false;
};
}
