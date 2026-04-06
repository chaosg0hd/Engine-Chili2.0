#pragma once

#include "../../core/module.hpp"
#include "platform_window.hpp"

#include <windows.h>

#include <string>
#include <vector>

class EngineContext;

class PlatformModule : public IModule
{
public:
    PlatformModule();
    ~PlatformModule();

    const char* GetName() const override;

    bool Initialize(EngineContext& context) override;
    void Startup(EngineContext& context) override;
    void Update(EngineContext& context, float deltaTime) override;
    void Shutdown(EngineContext& context) override;

    bool IsInitialized() const;
    bool IsStarted() const;

    bool IsWindowOpen() const;
    bool IsWindowActive() const;
    HWND GetWindowHandle() const;
    int GetWindowWidth() const;
    int GetWindowHeight() const;

    void PollEvents();

    const std::vector<PlatformWindow::Event>& GetEvents() const;
    void ClearEvents();

    const std::string& GetPlatformName() const;

    void SetOverlayText(const std::wstring& text);
    const std::wstring& GetOverlayText() const;
    void SetWindowTitle(const std::wstring& title);

private:
    bool m_initialized = false;
    bool m_started = false;

    std::string m_platformName = "Windows";
    PlatformWindow* m_window = nullptr;
};
