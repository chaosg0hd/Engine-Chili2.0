#pragma once

#include "../../core/module.hpp"
#include "iplatform_service.hpp"

#include <windows.h>

#include <string>

class EngineContext;

class PlatformModule : public IModule, public IPlatformService
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
    bool IsStarted() const override;

    bool IsWindowOpen() const override;
    bool IsWindowActive() const override;
    bool IsWindowMaximized() const override;
    bool IsWindowMinimized() const override;
    HWND GetWindowHandle() const override;
    RenderSurface GetRenderSurface() const override;
    int GetWindowWidth() const override;
    int GetWindowHeight() const override;
    float GetWindowAspectRatio() const override;

    void PollEvents() override;

    const std::vector<PlatformWindow::Event>& GetEvents() const override;
    void ClearEvents() override;

    const std::string& GetPlatformName() const override;

    void SetOverlayText(const std::wstring& text) override;
    const std::wstring& GetOverlayText() const override;
    void SetWindowTitle(const std::wstring& title) override;
    std::wstring GetWindowTitle() const override;
    void MaximizeWindow() override;
    void RestoreWindow() override;
    void MinimizeWindow() override;
    void SetWindowSize(int width, int height) override;
    void SetCursorVisible(bool visible) override;
    bool IsCursorVisible() const override;
    void SetCursorLocked(bool locked) override;
    bool IsCursorLocked() const override;

private:
    bool m_initialized = false;
    bool m_started = false;

    std::string m_platformName = "Windows";
    PlatformWindow* m_window = nullptr;
};
