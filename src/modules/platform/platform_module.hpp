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
    bool IsWindowMaximized() const;
    bool IsWindowMinimized() const;
    HWND GetWindowHandle() const;
    int GetWindowWidth() const;
    int GetWindowHeight() const;
    float GetWindowAspectRatio() const;

    void PollEvents();

    const std::vector<PlatformWindow::Event>& GetEvents() const;
    void ClearEvents();

    const std::string& GetPlatformName() const;

    void SetOverlayText(const std::wstring& text);
    const std::wstring& GetOverlayText() const;
    void SetWindowTitle(const std::wstring& title);
    std::wstring GetWindowTitle() const;
    void MaximizeWindow();
    void RestoreWindow();
    void MinimizeWindow();
    void SetWindowSize(int width, int height);
    void SetCursorVisible(bool visible);
    bool IsCursorVisible() const;
    void SetCursorLocked(bool locked);
    bool IsCursorLocked() const;

private:
    bool m_initialized = false;
    bool m_started = false;

    std::string m_platformName = "Windows";
    PlatformWindow* m_window = nullptr;
};
