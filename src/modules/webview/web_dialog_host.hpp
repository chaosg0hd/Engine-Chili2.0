#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include "../../core/engine_core.hpp"

#include <windows.h>

#include <string>

struct ICoreWebView2;
struct ICoreWebView2Controller;
struct ICoreWebView2Environment;

class WebDialogHost
{
public:
    bool Initialize(HWND engineWindowHandle, const WebDialogDesc& desc);
    void TickLayout(const RECT& engineClientRect);
    void Shutdown();

    bool SetContentPath(const std::string& contentPath);
    bool SetDockMode(WebDialogDockMode dockMode, int dockSize);
    bool SetBounds(const WebDialogRect& bounds);
    bool SetVisible(bool visible);

    bool IsReady() const;
    bool IsOpen() const;
    bool IsVisible() const;
    WebDialogRect GetBounds() const;
    WebDialogDockMode GetDockMode() const;
    int GetDockSize() const;

    void HandleEnvironmentCreated(HRESULT result, ICoreWebView2Environment* environment);
    void HandleControllerCreated(HRESULT result, ICoreWebView2Controller* controller);

private:
    bool EnsureComApartment();
    bool LoadWebViewLoader();
    bool CreateHostWindow();
    bool CreateChildWindow(const RECT& bounds);
    bool CreateFloatingWindow(const WebDialogRect& bounds);
    void DestroyHostWindow();
    void ReleaseWebViewObjects();
    void ResetInitializationState();
    void UpdateControllerBounds();
    void ApplyCurrentVisibility();
    RECT ComputeDockedBounds(const RECT& engineClientRect) const;
    RECT ToRect(const WebDialogRect& bounds) const;
    WebDialogRect ToDialogRect(const RECT& bounds) const;
    std::wstring BuildContentUri() const;
    void ShowFallbackText(const wchar_t* text);
    bool NavigateToCurrentContent();
    bool RecreateHostWindow();
    DWORD BuildFloatingWindowStyle() const;
    static bool EnsureWindowClassRegistered();
    static LRESULT CALLBACK WindowProcSetup(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK WindowProcThunk(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    LRESULT HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

private:
    HWND m_engineWindowHandle = nullptr;
    HWND m_hostWindowHandle = nullptr;
    RECT m_lastBounds{};
    std::string m_contentPath;
    std::wstring m_title;
    WebDialogDockMode m_dockMode = WebDialogDockMode::Floating;
    WebDialogRect m_bounds{};
    int m_dockSize = 360;
    bool m_visible = true;
    bool m_resizable = true;
    bool m_alwaysOnTop = false;
    bool m_isFloatingWindow = false;

    HMODULE m_webViewLoaderModule = nullptr;
    ICoreWebView2Environment* m_environment = nullptr;
    ICoreWebView2Controller* m_controller = nullptr;
    ICoreWebView2* m_webView = nullptr;

    bool m_initialized = false;
    bool m_ready = false;
    bool m_open = false;
    bool m_comInitialized = false;
};
