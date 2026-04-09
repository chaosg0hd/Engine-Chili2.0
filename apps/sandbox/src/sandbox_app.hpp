#pragma once

#include "prototypes/presentation/frame.hpp"

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
        unsigned int objectCount = 0;
        bool rotationPaused = false;
        bool cameraOrbitEnabled = true;
    };

private:
    void UpdateFrame(EngineCore& core);
    void UpdateLogic(EngineCore& core);
    FramePrototype BuildDemoFrame() const;
    std::wstring BuildPresentationOverlay(const EngineCore& core) const;
    void InitializeSandboxLog(EngineCore& core);
    void AppendSandboxLogLine(const std::string& line);
    void FlushSandboxLog(EngineCore& core, bool force);

private:
    SandboxState m_state;
    std::string m_sandboxLogPath = "logs/sandbox_3d_visibility.txt";
    std::string m_sandboxLogBuffer;
    double m_nextSandboxLogFlushTime = 0.0;
};
