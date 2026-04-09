#include "sandbox_app.hpp"

#include "core/engine_core.hpp"
#include "prototypes/render/render_item.hpp"
#include "prototypes/render/render_pass.hpp"
#include "prototypes/render/render_view.hpp"

#include <algorithm>
#include <cmath>
#include <string>

bool SandboxApp::Run()
{
    EngineCore core;

    if (!core.Initialize())
    {
        return false;
    }

    core.LogInfo(
        std::string("SandboxApp: Minimal sandbox ready | file logging = ") +
        (core.IsFileLoggingAvailable() ? "true" : "false") +
        " | log path = " +
        core.GetLogFilePath()
    );

    core.LogInfo("SandboxApp: Active harness = minimal logic/presentation test.");
    core.LogInfo("SandboxApp: Legacy feature harness archived under apps/sandbox/archive.");

    m_state = SandboxState{};
    m_state.parallelLaneCount = std::max(1U, core.GetJobWorkerCount());
    core.LogInfo(
        std::string("SandboxApp: Parallel lane count initialized to max worker count = ") +
        std::to_string(m_state.parallelLaneCount)
    );
    core.SetFrameCallback(
        [this](EngineCore& callbackCore)
        {
            UpdateFrame(callbackCore);
        });

    const bool success = core.Run();
    core.Shutdown();
    return success;
}

void SandboxApp::UpdateFrame(EngineCore& core)
{
    UpdateLogic(core);
    const std::uint32_t red = static_cast<std::uint32_t>(18.0 + (m_state.pulse * 28.0));
    const std::uint32_t green = static_cast<std::uint32_t>(28.0 + (m_state.pulse * 44.0));
    const std::uint32_t blue = static_cast<std::uint32_t>(46.0 + (m_state.pulse * 64.0));
    const std::uint32_t clearColor =
        0xFF000000u |
        (red << 16) |
        (green << 8) |
        blue;
    core.ClearFrame(clearColor);
    QueueParallelFrameBuild(core);
    SubmitSelectedFrame(core);
    core.SetWindowOverlayText(BuildPresentationOverlay(core));
}

void SandboxApp::UpdateLogic(EngineCore& core)
{
    ++m_state.frameCount;
    m_state.totalTime = core.GetTotalTime();
    m_state.pulse = (std::sin(m_state.totalTime * 1.75) + 1.0) * 0.5;

    const unsigned int maxLaneCount = std::max(1U, core.GetJobWorkerCount());
    if (m_state.parallelLaneCount != maxLaneCount)
    {
        m_state.parallelLaneCount = maxLaneCount;
    }

    constexpr unsigned char kTabKey = 9;
    if (core.WasKeyPressed(kTabKey))
    {
        core.LogInfo(
            std::string("SandboxApp: Parallel lane count refreshed to max worker count = ") +
            std::to_string(m_state.parallelLaneCount)
        );
    }
}

void SandboxApp::QueueParallelFrameBuild(EngineCore& core)
{
    {
        std::lock_guard<std::mutex> lock(m_pendingFrameMutex);
        if (m_pendingFrame.generation != 0 && !m_pendingFrame.ready)
        {
            return;
        }
    }

    const unsigned long long nextGeneration = m_state.buildRequestGeneration + 1;
    const double pulse = m_state.pulse;

    {
        std::lock_guard<std::mutex> lock(m_pendingFrameMutex);
        m_pendingFrame = PendingFrameBuild{};
        m_pendingFrame.generation = nextGeneration;
        m_pendingFrame.pulse = pulse;
        m_pendingFrame.passes.resize(m_state.parallelLaneCount);
        for (std::size_t passIndex = 0; passIndex < m_pendingFrame.passes.size(); ++passIndex)
        {
            if (passIndex < m_lastPresentedFrame.passes.size())
            {
                m_pendingFrame.passes[passIndex] = m_lastPresentedFrame.passes[passIndex];
            }
        }
        m_pendingFrame.remainingPasses = m_state.parallelLaneCount;
        m_pendingFrame.ready = false;
    }

    bool allSubmitted = true;
    for (unsigned int laneIndex = 0; laneIndex < m_state.parallelLaneCount; ++laneIndex)
    {
        const bool submitted = core.SubmitJob(
            [this, nextGeneration, pulse, laneIndex]()
            {
                RenderPassPrototype pass = BuildAsyncTestPass(nextGeneration, pulse, laneIndex);
                bool completedPassStored = false;

                {
                    std::lock_guard<std::mutex> lock(m_pendingFrameMutex);
                    if (m_pendingFrame.generation != nextGeneration || m_pendingFrame.passes.size() <= laneIndex)
                    {
                        m_buildJobsInFlight.fetch_sub(1);
                        return;
                    }

                    m_pendingFrame.passes[static_cast<std::size_t>(laneIndex)] = std::move(pass);
                    completedPassStored = true;
                    if (m_pendingFrame.remainingPasses > 0U)
                    {
                        --m_pendingFrame.remainingPasses;
                    }

                    if (m_pendingFrame.remainingPasses == 0U)
                    {
                        m_pendingFrame.ready = true;
                    }
                }

                if (completedPassStored)
                {
                    m_completedPassBuildCount.fetch_add(1);
                }
                m_buildJobsInFlight.fetch_sub(1);
            }
        );

        if (!submitted)
        {
            allSubmitted = false;
            core.LogWarn("SandboxApp: Parallel frame pass build request was rejected by the job system.");
            break;
        }

        m_dispatchedPassBuildCount.fetch_add(1);
        m_buildJobsInFlight.fetch_add(1);
    }

    if (!allSubmitted)
    {
        std::lock_guard<std::mutex> lock(m_pendingFrameMutex);
        m_pendingFrame = PendingFrameBuild{};
        ++m_state.droppedFrameGenerationCount;
        m_buildJobsInFlight.store(0);
        return;
    }

    m_state.buildRequestGeneration = nextGeneration;
}

void SandboxApp::SubmitSelectedFrame(EngineCore& core)
{
    PendingFrameBuild readyFrame;
    {
        std::lock_guard<std::mutex> lock(m_pendingFrameMutex);
        if (m_pendingFrame.generation == 0 || !m_pendingFrame.ready || m_pendingFrame.generation <= m_state.submittedFrameGeneration)
        {
            return;
        }

        readyFrame = m_pendingFrame;
        m_pendingFrame = PendingFrameBuild{};
    }

    RenderFramePrototype frame;
    frame.passes = std::move(readyFrame.passes);

    core.SubmitRenderFrame(frame);
    m_lastPresentedFrame = frame;
    m_state.submittedFrameGeneration = readyFrame.generation;
    m_state.completedFrameGeneration = readyFrame.generation;
    m_state.selectedPassCount = static_cast<unsigned int>(frame.passes.size());
}

RenderPassPrototype SandboxApp::BuildAsyncTestPass(unsigned long long generation, double pulse, unsigned int laneIndex)
{
    double workload = pulse + static_cast<double>(laneIndex) * 0.03125;
    for (int iteration = 0; iteration < 20000; ++iteration)
    {
        const double angle = workload + (static_cast<double>(iteration) * 0.00015);
        workload += (std::sin(angle) * 0.00035) + (std::cos(angle * 0.5) * 0.00020);
    }

    RenderItemPrototype item;
    item.kind = RenderItemKind::Object3D;
    item.object3D.mesh.handle = static_cast<RenderMeshHandle>(((generation + laneIndex) % 3U) + 1U);

    const double lanePulse = std::fmod(std::abs(workload), 1.0);
    const std::uint32_t red = static_cast<std::uint32_t>(48.0 + (lanePulse * 191.0));
    const std::uint32_t green = static_cast<std::uint32_t>(80.0 + ((1.0 - lanePulse) * 143.0));
    const std::uint32_t blue = 96U + ((laneIndex * 29U) % 120U);
    item.object3D.material.baseColor =
        0xFF000000u |
        (red << 16) |
        (green << 8) |
        blue;

    RenderViewPrototype view;
    view.kind = RenderViewKind::Scene3D;
    view.items.push_back(item);

    RenderPassPrototype pass;
    pass.kind = (laneIndex == 0U) ? RenderPassKind::Scene : RenderPassKind::Overlay;
    pass.views.push_back(view);
    return pass;
}

std::wstring SandboxApp::BuildPresentationOverlay(const EngineCore& core) const
{
    std::wstring overlay =
        L"SANDBOX PARALLEL FRAME TEST\n"
        L"  Frames: " + std::to_wstring(m_state.frameCount) + L"\n"
        L"  Uptime: " + std::to_wstring(m_state.totalTime) + L"s\n"
        L"  Pulse: " + std::to_wstring(m_state.pulse) + L"\n"
        L"  Worker Count: " + std::to_wstring(core.GetJobWorkerCount()) + L"\n"
        L"  Parallel Lanes: " + std::to_wstring(m_state.parallelLaneCount) + L"\n"
        L"  Parallel Build: " + std::wstring(m_buildJobsInFlight.load() > 0U ? L"In Flight" : L"Idle") + L"\n"
        L"  Frame Build/Submit: " + std::to_wstring(m_state.completedFrameGeneration) + L" / " + std::to_wstring(m_state.submittedFrameGeneration) + L"\n"
        L"  Pass Jobs Dispatch/Done: " + std::to_wstring(m_dispatchedPassBuildCount.load()) + L" / " + std::to_wstring(m_completedPassBuildCount.load()) + L"\n"
        L"  Selected Passes: " + std::to_wstring(m_state.selectedPassCount) + L"\n"
        L"  Baseline Passes: " + std::to_wstring(static_cast<unsigned int>(m_lastPresentedFrame.passes.size())) + L"\n"
        L"  Dropped Generations: " + std::to_wstring(m_state.droppedFrameGenerationCount) + L"\n"
        L"  Render Items: " + std::to_wstring(core.GetRenderSubmittedItemCount()) + L"\n"
        L"  Tab: refresh lane count from worker max\n"
        L"  Escape: exit\n";
    return overlay;
}
