#include "sandbox_app.hpp"

#include "core/engine_core.hpp"

#include <algorithm>
#include <utility>

namespace
{
    constexpr bool UseRegionRenderPatchSmokeTest = false;
}

bool SandboxApp::Run()
{
    EngineCore core;

    if (!core.Initialize())
    {
        return false;
    }

    ConfigureStrategy();
    core.CreateDirectory("logs");
    core.WriteTextFile(
        "logs/progressive_hex_algo_traversal.txt",
        "Progressive hex algorithm traversal log\npending grid generation...\n");
    core.LogInfo("SandboxApp: Progressive hex render strategy sandbox ready.");

    m_state = SandboxState{};

    core.SetFrameCallback(
        [this](EngineCore& callbackCore)
        {
            UpdateFrame(callbackCore);
        });

    const bool success = core.Run();
    core.Shutdown();
    return success;
}

void SandboxApp::ConfigureStrategy()
{
    render::ProgressiveHexRenderControllerOptions options;
    options.overlayTitle = L"PROGRESSIVE HEX CUBE SAMPLE";
    options.sampleLabel = L"moving cube scene";
    options.presentationOptions.mode = UseRegionRenderPatchSmokeTest
        ? ProgressiveHexRenderPresentationModePrototype::RegionRectangles
        : ProgressiveHexRenderPresentationModePrototype::NativeHexPatches;
    options.presentationOptions.maxRegionPatches = 9000U;

    ProgressiveHexRenderConfigPrototype& config = options.strategyConfig;
    config.hexSize = 0.04f;
    config.centerRegionFactor = 0.4f;
    config.maxPassesPerFrame = 20000U;
    config.stepBatchSize = 2048U;
    config.maxRenderStepSeconds = 0.003;
    config.fillPassesPerSecond = 60.0;
    config.useFullResourceFill = true;
    config.persistentPaint = true;
    config.useParallelLaneOffsets = true;
    config.useRegionOwnershipColors = false;
    config.showUnfilledGrid = true;
    config.renderTerminalHexDetail = false;
    config.fadeByTimeSincePass = true;
    config.assignSubregions = false;
    config.buildFillOrder = true;
    config.highlightMajorRegionCenters = true;
    config.patchSplatScale = 1.02f;
    config.mainCenterStealScale = 0.0f;
    config.maxSubdivisionDepthClamp = 0U;
    config.mainCenterMaxSubdivisionDepthClamp = 0U;
    config.mainCenterChildRadiusScale = 0.0f;
    config.passFadeSeconds = 1.50f;
    config.passFadeFloor = 0.12f;

    m_renderController.Configure(options);
    m_sampleScene.SetTheoreticalWorkIterations(m_state.theoreticalWorkIterations);
    m_renderController.SetSampleProvider(
        [this](float screenX, float screenY, double time) -> std::uint32_t
        {
            return m_sampleScene.Sample(screenX, screenY, time);
        });
}

void SandboxApp::UpdateFrame(EngineCore& core)
{
    UpdateLogic(core);

    core.ClearFrame(0xFF080B0Eu);
    core.SubmitRenderFrame(m_renderController.BuildFrame());
    core.SetWindowOverlayText(m_renderController.BuildOverlayText());
}

void SandboxApp::UpdateLogic(EngineCore& core)
{
    render::ProgressiveHexRenderControllerContext context;
    context.screenWidth = core.GetFrameWidth();
    context.screenHeight = core.GetFrameHeight();
    context.aspectRatio = static_cast<float>(core.GetFrameAspectRatio());
    if (context.aspectRatio <= 0.001f)
    {
        context.aspectRatio = 1.0f;
    }
    context.totalTime = core.GetTotalTime();
    context.deltaTime = core.GetDeltaTime();
    context.behindSchedule = core.IsBehindSchedule();
    context.lastFrameWaitDuration = core.GetLastFrameWaitDuration();
    context.targetFrameTime = core.GetTargetFrameTime();
    context.lastFrameWorkDuration = core.GetLastFrameWorkDuration();
    context.idleWorkerCount = std::max(1U, core.GetIdleJobWorkerCount());

    if (!m_state.traversalLogWritten && m_renderController.GetStrategy().GetStats().generatedHexCount > 0U)
    {
        WriteTraversalLog(core);
    }

    if (core.WasKeyPressed('R'))
    {
        m_renderController.Reset();
        m_renderController.Update(context);
        m_state.traversalLogWritten = false;
        m_state.passLogWritten = false;
        WriteTraversalLog(core);
        core.LogInfo("[PROGRESSIVE-HEX] Regenerated strategy state.");
    }

    m_renderController.Update(
        context,
        [&core](ProgressiveHexRenderJobFunctionPrototype job)
        {
            return core.SubmitJob(std::move(job));
        },
        [&core]()
        {
            core.WaitForAllJobs();
        });

    if (m_renderController.HasReachedMaxDepth() && !m_state.maxDepthSignalLogged)
    {
        core.LogInfo("[PROGRESSIVE-HEX] Runtime pass list complete; cycling over mapped order again.");
        m_state.maxDepthSignalLogged = true;
    }
    if (!m_renderController.HasReachedMaxDepth())
    {
        m_state.maxDepthSignalLogged = false;
    }
    UpdateCenterPassLog(core);
}

void SandboxApp::WriteTraversalLog(EngineCore& core)
{
    const render::ProgressiveHexRenderController::DebugLogBundle logs =
        m_renderController.BuildDebugLogBundle();

    core.CreateDirectory("logs");
    if (core.WriteTextFile(
        "logs/progressive_hex_algo_traversal.txt",
        logs.traversalLog))
    {
        m_state.traversalLogWritten = true;
        core.LogInfo("SandboxApp: Wrote logs/progressive_hex_algo_traversal.txt.");
    }

    if (core.WriteTextFile(
        "logs/progressive_hex_runtime_order.csv",
        logs.runtimeOrderLog))
    {
        core.LogInfo("SandboxApp: Wrote logs/progressive_hex_runtime_order.csv.");
    }
}

void SandboxApp::UpdateCenterPassLog(EngineCore& core)
{
    const bool freshCycle = !m_state.passLogWritten;

    if (!freshCycle)
    {
        return;
    }

    core.CreateDirectory("logs");
    if (core.WriteTextFile(
        "logs/progressive_hex_center_passes.csv",
        m_renderController.BuildDebugLogBundle().centerPassLog))
    {
        if (freshCycle)
        {
            core.LogInfo("[PROGRESSIVE-HEX] Wrote master center pass list.");
        }

        m_state.passLogWritten = true;
    }
}
