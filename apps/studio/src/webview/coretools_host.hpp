#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>

#include <string>

struct ICoreWebView2;
struct ICoreWebView2Controller;
struct ICoreWebView2Environment;

class CoreToolsHost
{
public:
    bool Initialize(HWND parentWindowHandle, const RECT& initialBounds, const std::string& contentPath);
    void Resize(const RECT& bounds);
    void Shutdown();

    bool IsInitialized() const;
    bool IsReady() const;

    void HandleEnvironmentCreated(HRESULT result, ICoreWebView2Environment* environment);
    void HandleControllerCreated(HRESULT result, ICoreWebView2Controller* controller);

private:
    bool EnsureComApartment();
    bool CreateHostWindow();
    bool LoadWebViewLoader();
    void DestroyHostWindow();
    void ReleaseWebViewObjects();
    void ResetInitializationState();
    void UpdateControllerBounds();
    std::wstring BuildContentUri() const;
    void ShowFallbackText(const wchar_t* text);

private:
    HWND m_parentWindowHandle = nullptr;
    HWND m_hostWindowHandle = nullptr;
    RECT m_bounds{};
    std::string m_contentPath;

    HMODULE m_webViewLoaderModule = nullptr;
    ICoreWebView2Environment* m_environment = nullptr;
    ICoreWebView2Controller* m_controller = nullptr;
    ICoreWebView2* m_webView = nullptr;

    bool m_initialized = false;
    bool m_ready = false;
    bool m_comInitialized = false;
};
