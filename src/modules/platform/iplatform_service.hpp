#pragma once

#include "platform_window.hpp"

#include <windef.h>

#include <cstdint>
#include <string>
#include <vector>

struct RenderSurface
{
    void* nativeHandle = nullptr;
    std::uint32_t width = 0;
    std::uint32_t height = 0;
};

class IPlatformService
{
public:
    virtual ~IPlatformService() = default;

    virtual bool IsWindowOpen() const = 0;
    virtual bool IsWindowActive() const = 0;
    virtual bool IsWindowMaximized() const = 0;
    virtual bool IsWindowMinimized() const = 0;
    virtual HWND GetWindowHandle() const = 0;
    virtual RenderSurface GetRenderSurface() const = 0;
    virtual int GetWindowWidth() const = 0;
    virtual int GetWindowHeight() const = 0;
    virtual float GetWindowAspectRatio() const = 0;

    virtual void PollEvents() = 0;
    virtual const std::vector<PlatformWindow::Event>& GetEvents() const = 0;
    virtual void ClearEvents() = 0;

    virtual const std::string& GetPlatformName() const = 0;

    virtual void SetOverlayText(const std::wstring& text) = 0;
    virtual const std::wstring& GetOverlayText() const = 0;
    virtual void SetWindowTitle(const std::wstring& title) = 0;
    virtual std::wstring GetWindowTitle() const = 0;
    virtual void MaximizeWindow() = 0;
    virtual void RestoreWindow() = 0;
    virtual void MinimizeWindow() = 0;
    virtual void SetWindowSize(int width, int height) = 0;
    virtual void SetCursorVisible(bool visible) = 0;
    virtual bool IsCursorVisible() const = 0;
    virtual void SetCursorLocked(bool locked) = 0;
    virtual bool IsCursorLocked() const = 0;
    virtual bool IsStarted() const = 0;
};
