#pragma once

#include "app/app_capabilities.hpp"
#include "core/engine_core.hpp"

#include <string>
#include <windef.h>

class EngineBridge
{
public:
    bool Initialize();
    void Shutdown();

    void LogInfo(const std::string& message);
    void LogWarn(const std::string& message);
    void LogError(const std::string& message);

    std::string GetStudioRootPath() const;
    std::string GetCoreToolsContentPath() const;
    std::string GetStudioTopBarContentPath() const;
    std::string BuildHelloMessage(const std::string& sender) const;
    std::string BuildStatusMessage() const;
    bool Tick();
    AppCapabilities& GetCapabilities();
    const AppCapabilities& GetCapabilities() const;
    HWND GetNativeWindowHandle() const;

    void RequestExit();
    bool ShouldExit() const;
    bool IsInitialized() const;

private:
    EngineCore m_core;
    bool m_initialized = false;
    bool m_exitRequested = false;
};
