#pragma once

#include "core/engine_core.hpp"

#include <string>

class EngineBridge
{
public:
    bool Initialize();
    void Shutdown();

    void LogInfo(const std::string& message);
    void LogWarn(const std::string& message);
    void LogError(const std::string& message);

    std::string GetStudioWebRootPath() const;
    std::string GetStudioWebUrl() const;
    std::string BuildHelloMessage(const std::string& sender) const;
    std::string BuildStatusMessage() const;
    bool Tick();

    void RequestExit();
    bool ShouldExit() const;
    bool IsInitialized() const;

private:
    EngineCore m_core;
    bool m_initialized = false;
    bool m_exitRequested = false;
};
