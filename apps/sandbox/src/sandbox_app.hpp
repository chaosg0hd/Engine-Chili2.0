#pragma once

#include "prototypes/render/render_frame.hpp"
#include "prototypes/render/render_pass.hpp"

#include <atomic>
#include <mutex>
#include <string>

class EngineCore;

class SandboxApp
{
public:
    bool Run();

private:
    struct SandboxState
    {
        unsigned long long frameCount = 0;
        double totalTime = 0.0;
        double pulse = 0.0;
        unsigned long long submittedFrameGeneration = 0;
        unsigned long long completedFrameGeneration = 0;
        unsigned long long buildRequestGeneration = 0;
        unsigned int selectedPassCount = 0;
        unsigned int parallelLaneCount = 2;
        unsigned long long droppedFrameGenerationCount = 0;
    };

    struct PendingFrameBuild
    {
        unsigned long long generation = 0;
        double pulse = 0.0;
        unsigned int remainingPasses = 0;
        bool ready = false;
        std::vector<RenderPassPrototype> passes;
    };

private:
    void UpdateFrame(EngineCore& core);
    void UpdateLogic(EngineCore& core);
    void QueueParallelFrameBuild(EngineCore& core);
    void SubmitSelectedFrame(EngineCore& core);
    static RenderPassPrototype BuildAsyncTestPass(unsigned long long generation, double pulse, unsigned int laneIndex);
    std::wstring BuildPresentationOverlay(const EngineCore& core) const;

private:
    SandboxState m_state;
    mutable std::mutex m_pendingFrameMutex;
    PendingFrameBuild m_pendingFrame;
    RenderFramePrototype m_lastPresentedFrame;
    std::atomic<unsigned long long> m_dispatchedPassBuildCount{0};
    std::atomic<unsigned long long> m_completedPassBuildCount{0};
    std::atomic<unsigned int> m_buildJobsInFlight{0};
};
