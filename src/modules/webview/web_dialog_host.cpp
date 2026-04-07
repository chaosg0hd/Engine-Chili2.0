#include "web_dialog_host.hpp"

#include <objbase.h>
#include <unknwn.h>

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <string>

struct EventRegistrationToken
{
    std::int64_t value;
};

enum COREWEBVIEW2_MOVE_FOCUS_REASON
{
    COREWEBVIEW2_MOVE_FOCUS_REASON_PROGRAMMATIC = 0,
    COREWEBVIEW2_MOVE_FOCUS_REASON_NEXT = 1,
    COREWEBVIEW2_MOVE_FOCUS_REASON_PREVIOUS = 2
};

struct ICoreWebView2CreateCoreWebView2ControllerCompletedHandler;

struct ICoreWebView2 : public IUnknown
{
    virtual HRESULT STDMETHODCALLTYPE get_Settings(IUnknown** settings) = 0;
    virtual HRESULT STDMETHODCALLTYPE get_Source(LPWSTR* uri) = 0;
    virtual HRESULT STDMETHODCALLTYPE Navigate(LPCWSTR uri) = 0;
    virtual HRESULT STDMETHODCALLTYPE NavigateToString(LPCWSTR htmlContent) = 0;
};

struct ICoreWebView2Controller : public IUnknown
{
    virtual HRESULT STDMETHODCALLTYPE get_IsVisible(BOOL* isVisible) = 0;
    virtual HRESULT STDMETHODCALLTYPE put_IsVisible(BOOL isVisible) = 0;
    virtual HRESULT STDMETHODCALLTYPE get_Bounds(RECT* bounds) = 0;
    virtual HRESULT STDMETHODCALLTYPE put_Bounds(RECT bounds) = 0;
    virtual HRESULT STDMETHODCALLTYPE get_ZoomFactor(double* zoomFactor) = 0;
    virtual HRESULT STDMETHODCALLTYPE put_ZoomFactor(double zoomFactor) = 0;
    virtual HRESULT STDMETHODCALLTYPE add_ZoomFactorChanged(IUnknown* eventHandler, EventRegistrationToken* token) = 0;
    virtual HRESULT STDMETHODCALLTYPE remove_ZoomFactorChanged(EventRegistrationToken token) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetBoundsAndZoomFactor(RECT bounds, double zoomFactor) = 0;
    virtual HRESULT STDMETHODCALLTYPE MoveFocus(COREWEBVIEW2_MOVE_FOCUS_REASON reason) = 0;
    virtual HRESULT STDMETHODCALLTYPE add_MoveFocusRequested(IUnknown* eventHandler, EventRegistrationToken* token) = 0;
    virtual HRESULT STDMETHODCALLTYPE remove_MoveFocusRequested(EventRegistrationToken token) = 0;
    virtual HRESULT STDMETHODCALLTYPE add_GotFocus(IUnknown* eventHandler, EventRegistrationToken* token) = 0;
    virtual HRESULT STDMETHODCALLTYPE remove_GotFocus(EventRegistrationToken token) = 0;
    virtual HRESULT STDMETHODCALLTYPE add_LostFocus(IUnknown* eventHandler, EventRegistrationToken* token) = 0;
    virtual HRESULT STDMETHODCALLTYPE remove_LostFocus(EventRegistrationToken token) = 0;
    virtual HRESULT STDMETHODCALLTYPE add_AcceleratorKeyPressed(IUnknown* eventHandler, EventRegistrationToken* token) = 0;
    virtual HRESULT STDMETHODCALLTYPE remove_AcceleratorKeyPressed(EventRegistrationToken token) = 0;
    virtual HRESULT STDMETHODCALLTYPE get_ParentWindow(HWND* parentWindow) = 0;
    virtual HRESULT STDMETHODCALLTYPE put_ParentWindow(HWND parentWindow) = 0;
    virtual HRESULT STDMETHODCALLTYPE NotifyParentWindowPositionChanged() = 0;
    virtual HRESULT STDMETHODCALLTYPE Close() = 0;
    virtual HRESULT STDMETHODCALLTYPE get_CoreWebView2(ICoreWebView2** coreWebView2) = 0;
};

struct ICoreWebView2Environment : public IUnknown
{
    virtual HRESULT STDMETHODCALLTYPE CreateCoreWebView2Controller(
        HWND parentWindow,
        ICoreWebView2CreateCoreWebView2ControllerCompletedHandler* handler) = 0;
};

struct ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler : public IUnknown
{
    virtual HRESULT STDMETHODCALLTYPE Invoke(HRESULT result, ICoreWebView2Environment* environment) = 0;
};

struct ICoreWebView2CreateCoreWebView2ControllerCompletedHandler : public IUnknown
{
    virtual HRESULT STDMETHODCALLTYPE Invoke(HRESULT result, ICoreWebView2Controller* controller) = 0;
};

namespace
{
    constexpr wchar_t kWebDialogWindowClassName[] = L"ProjectEngineWebDialogHostWindow";

    using CreateCoreWebView2EnvironmentWithOptionsFn = HRESULT(STDAPICALLTYPE*)(
        PCWSTR browserExecutableFolder,
        PCWSTR userDataFolder,
        IUnknown* environmentOptions,
        ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler* environmentCreatedHandler);

    template<typename T>
    void SafeRelease(T*& object)
    {
        if (object)
        {
            object->Release();
            object = nullptr;
        }
    }

    std::wstring ToWideString(const std::string& value)
    {
        return std::wstring(value.begin(), value.end());
    }

    std::wstring EncodeFilePathSegment(const std::wstring& value)
    {
        std::wstring encoded;
        encoded.reserve(value.size() + 16);

        auto appendHex = [&encoded](wchar_t valueToEncode)
        {
            static const wchar_t* kHex = L"0123456789ABCDEF";
            encoded.push_back(L'%');
            encoded.push_back(kHex[(valueToEncode >> 4) & 0xF]);
            encoded.push_back(kHex[valueToEncode & 0xF]);
        };

        for (wchar_t character : value)
        {
            if ((character >= L'a' && character <= L'z') ||
                (character >= L'A' && character <= L'Z') ||
                (character >= L'0' && character <= L'9') ||
                character == L'/' ||
                character == L':' ||
                character == L'-' ||
                character == L'_' ||
                character == L'.')
            {
                encoded.push_back(character);
                continue;
            }

            if (character == L'\\')
            {
                encoded.push_back(L'/');
                continue;
            }

            appendHex(character & 0xFF);
        }

        return encoded;
    }

    bool FileExists(const std::wstring& path)
    {
        if (path.empty())
        {
            return false;
        }

        const DWORD attributes = GetFileAttributesW(path.c_str());
        return attributes != INVALID_FILE_ATTRIBUTES &&
            (attributes & FILE_ATTRIBUTE_DIRECTORY) == 0;
    }

    CreateCoreWebView2EnvironmentWithOptionsFn GetCreateEnvironmentFunction(HMODULE moduleHandle)
    {
        if (!moduleHandle)
        {
            return nullptr;
        }

        FARPROC rawFunction = GetProcAddress(moduleHandle, "CreateCoreWebView2EnvironmentWithOptions");
        if (!rawFunction)
        {
            return nullptr;
        }

        CreateCoreWebView2EnvironmentWithOptionsFn typedFunction = nullptr;
        static_assert(sizeof(typedFunction) == sizeof(rawFunction), "Function pointer size mismatch.");
        std::memcpy(&typedFunction, &rawFunction, sizeof(typedFunction));
        return typedFunction;
    }

    class EnvironmentCompletedHandler final : public ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler
    {
    public:
        explicit EnvironmentCompletedHandler(WebDialogHost* owner)
            : m_owner(owner)
        {
        }

        HRESULT STDMETHODCALLTYPE QueryInterface(REFIID, void** object) override
        {
            if (!object)
            {
                return E_POINTER;
            }

            *object = this;
            AddRef();
            return S_OK;
        }

        ULONG STDMETHODCALLTYPE AddRef() override
        {
            return static_cast<ULONG>(InterlockedIncrement(&m_referenceCount));
        }

        ULONG STDMETHODCALLTYPE Release() override
        {
            const ULONG count = static_cast<ULONG>(InterlockedDecrement(&m_referenceCount));
            if (count == 0)
            {
                delete this;
            }

            return count;
        }

        HRESULT STDMETHODCALLTYPE Invoke(HRESULT result, ICoreWebView2Environment* environment) override
        {
            if (m_owner)
            {
                m_owner->HandleEnvironmentCreated(result, environment);
            }

            return S_OK;
        }

    private:
        WebDialogHost* m_owner = nullptr;
        long m_referenceCount = 1;
    };

    class ControllerCompletedHandler final : public ICoreWebView2CreateCoreWebView2ControllerCompletedHandler
    {
    public:
        explicit ControllerCompletedHandler(WebDialogHost* owner)
            : m_owner(owner)
        {
        }

        HRESULT STDMETHODCALLTYPE QueryInterface(REFIID, void** object) override
        {
            if (!object)
            {
                return E_POINTER;
            }

            *object = this;
            AddRef();
            return S_OK;
        }

        ULONG STDMETHODCALLTYPE AddRef() override
        {
            return static_cast<ULONG>(InterlockedIncrement(&m_referenceCount));
        }

        ULONG STDMETHODCALLTYPE Release() override
        {
            const ULONG count = static_cast<ULONG>(InterlockedDecrement(&m_referenceCount));
            if (count == 0)
            {
                delete this;
            }

            return count;
        }

        HRESULT STDMETHODCALLTYPE Invoke(HRESULT result, ICoreWebView2Controller* controller) override
        {
            if (m_owner)
            {
                m_owner->HandleControllerCreated(result, controller);
            }

            return S_OK;
        }

    private:
        WebDialogHost* m_owner = nullptr;
        long m_referenceCount = 1;
    };
}

bool WebDialogHost::Initialize(HWND engineWindowHandle, const WebDialogDesc& desc)
{
    if (m_initialized)
    {
        return true;
    }

    if (!engineWindowHandle)
    {
        return false;
    }

    m_engineWindowHandle = engineWindowHandle;
    m_contentPath = desc.contentPath;
    m_title = desc.title.empty() ? L"Web Dialog" : desc.title;
    m_dockMode = desc.dockMode;
    m_bounds = desc.rect;
    m_dockSize = desc.dockSize;
    m_visible = desc.visible;
    m_resizable = desc.resizable;
    m_alwaysOnTop = desc.alwaysOnTop;
    m_ready = false;
    m_open = false;

    if (!EnsureComApartment())
    {
        return false;
    }

    if (!LoadWebViewLoader())
    {
        return false;
    }

    if (!CreateHostWindow())
    {
        return false;
    }

    const auto createEnvironment = GetCreateEnvironmentFunction(m_webViewLoaderModule);
    if (!createEnvironment)
    {
        ShowFallbackText(L"CreateCoreWebView2EnvironmentWithOptions was not found.");
        ResetInitializationState();
        return false;
    }

    auto* handler = new EnvironmentCompletedHandler(this);
    const HRESULT result = createEnvironment(nullptr, nullptr, nullptr, handler);
    handler->Release();

    if (FAILED(result))
    {
        ShowFallbackText(L"Failed to start the WebView2 environment.");
        ResetInitializationState();
        return false;
    }

    m_initialized = true;
    m_open = true;
    return true;
}

void WebDialogHost::TickLayout(const RECT& engineClientRect)
{
    if (!m_hostWindowHandle || m_isFloatingWindow || m_dockMode == WebDialogDockMode::Floating)
    {
        return;
    }

    const RECT target = ComputeDockedBounds(engineClientRect);
    if (EqualRect(&target, &m_lastBounds))
    {
        return;
    }

    m_lastBounds = target;
    SetWindowPos(
        m_hostWindowHandle,
        nullptr,
        target.left,
        target.top,
        std::max<LONG>(0, target.right - target.left),
        std::max<LONG>(0, target.bottom - target.top),
        SWP_NOACTIVATE | SWP_NOZORDER);

    UpdateControllerBounds();
}

void WebDialogHost::Shutdown()
{
    ReleaseWebViewObjects();
    DestroyHostWindow();

    if (m_webViewLoaderModule)
    {
        FreeLibrary(m_webViewLoaderModule);
        m_webViewLoaderModule = nullptr;
    }

    if (m_comInitialized)
    {
        CoUninitialize();
        m_comInitialized = false;
    }

    m_engineWindowHandle = nullptr;
    m_contentPath.clear();
    m_title.clear();
    ZeroMemory(&m_lastBounds, sizeof(m_lastBounds));
    m_initialized = false;
    m_ready = false;
    m_open = false;
}

bool WebDialogHost::SetContentPath(const std::string& contentPath)
{
    m_contentPath = contentPath;
    return NavigateToCurrentContent();
}

bool WebDialogHost::SetDockMode(WebDialogDockMode dockMode, int dockSize)
{
    m_dockMode = dockMode;
    m_dockSize = dockSize;
    return RecreateHostWindow();
}

bool WebDialogHost::SetBounds(const WebDialogRect& bounds)
{
    m_bounds = bounds;

    if (!m_hostWindowHandle)
    {
        return false;
    }

    if (m_isFloatingWindow || m_dockMode == WebDialogDockMode::ManualChild)
    {
        const RECT rect = ToRect(bounds);
        m_lastBounds = rect;
        SetWindowPos(
            m_hostWindowHandle,
            m_alwaysOnTop ? HWND_TOPMOST : nullptr,
            rect.left,
            rect.top,
            std::max<LONG>(0, rect.right - rect.left),
            std::max<LONG>(0, rect.bottom - rect.top),
            SWP_NOACTIVATE | (m_alwaysOnTop ? 0 : SWP_NOZORDER));
        UpdateControllerBounds();
        return true;
    }

    if (m_engineWindowHandle)
    {
        RECT clientRect{};
        GetClientRect(m_engineWindowHandle, &clientRect);
        TickLayout(clientRect);
        return true;
    }

    return false;
}

bool WebDialogHost::SetVisible(bool visible)
{
    m_visible = visible;
    ApplyCurrentVisibility();
    return true;
}

bool WebDialogHost::IsReady() const
{
    return m_ready;
}

bool WebDialogHost::IsOpen() const
{
    return m_open;
}

bool WebDialogHost::IsVisible() const
{
    return m_visible;
}

WebDialogRect WebDialogHost::GetBounds() const
{
    if (m_hostWindowHandle)
    {
        RECT bounds{};
        GetWindowRect(m_hostWindowHandle, &bounds);

        if (!m_isFloatingWindow && m_engineWindowHandle)
        {
            POINT topLeft{ bounds.left, bounds.top };
            POINT bottomRight{ bounds.right, bounds.bottom };
            ScreenToClient(m_engineWindowHandle, &topLeft);
            ScreenToClient(m_engineWindowHandle, &bottomRight);
            bounds.left = topLeft.x;
            bounds.top = topLeft.y;
            bounds.right = bottomRight.x;
            bounds.bottom = bottomRight.y;
        }

        return ToDialogRect(bounds);
    }

    return m_bounds;
}

WebDialogDockMode WebDialogHost::GetDockMode() const
{
    return m_dockMode;
}

int WebDialogHost::GetDockSize() const
{
    return m_dockSize;
}

void WebDialogHost::HandleEnvironmentCreated(HRESULT result, ICoreWebView2Environment* environment)
{
    m_ready = false;

    if (FAILED(result) || !environment)
    {
        ShowFallbackText(L"Failed to create the WebView2 environment.");
        ResetInitializationState();
        return;
    }

    SafeRelease(m_environment);
    m_environment = environment;
    m_environment->AddRef();

    auto* handler = new ControllerCompletedHandler(this);
    const HRESULT controllerResult = m_environment->CreateCoreWebView2Controller(m_hostWindowHandle, handler);
    handler->Release();

    if (FAILED(controllerResult))
    {
        ShowFallbackText(L"Failed to create the WebView2 controller.");
        ResetInitializationState();
    }
}

void WebDialogHost::HandleControllerCreated(HRESULT result, ICoreWebView2Controller* controller)
{
    m_ready = false;

    if (FAILED(result) || !controller)
    {
        ShowFallbackText(L"Failed to attach the web dialog to its host window.");
        ResetInitializationState();
        return;
    }

    SafeRelease(m_webView);
    SafeRelease(m_controller);
    m_controller = controller;
    m_controller->AddRef();

    if (FAILED(m_controller->put_IsVisible(TRUE)))
    {
        ShowFallbackText(L"Failed to make the web dialog visible.");
        ResetInitializationState();
        return;
    }

    UpdateControllerBounds();

    ICoreWebView2* webView = nullptr;
    if (FAILED(m_controller->get_CoreWebView2(&webView)) || !webView)
    {
        ShowFallbackText(L"Failed to access the WebView2 surface.");
        ResetInitializationState();
        return;
    }

    m_webView = webView;

    if (!NavigateToCurrentContent())
    {
        ResetInitializationState();
        return;
    }

    ApplyCurrentVisibility();
    m_ready = true;
}

bool WebDialogHost::EnsureComApartment()
{
    const HRESULT result = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (result == S_OK || result == S_FALSE)
    {
        m_comInitialized = true;
        return true;
    }

    if (result == RPC_E_CHANGED_MODE)
    {
        return true;
    }

    return false;
}

bool WebDialogHost::LoadWebViewLoader()
{
    if (m_webViewLoaderModule)
    {
        return true;
    }

    m_webViewLoaderModule = LoadLibraryW(L"WebView2Loader.dll");
    if (m_webViewLoaderModule)
    {
        return true;
    }

    m_webViewLoaderModule = LoadLibraryW(
        L"C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\Common7\\IDE\\CommonExtensions\\Microsoft\\Markdown\\runtimes\\win-x64\\native\\WebView2Loader.dll");
    return m_webViewLoaderModule != nullptr;
}

bool WebDialogHost::CreateHostWindow()
{
    if (m_hostWindowHandle)
    {
        return true;
    }

    if (!EnsureWindowClassRegistered())
    {
        return false;
    }

    if (m_dockMode == WebDialogDockMode::Floating)
    {
        return CreateFloatingWindow(m_bounds);
    }

    RECT initialBounds{};
    if (m_dockMode == WebDialogDockMode::ManualChild)
    {
        initialBounds = ToRect(m_bounds);
    }
    else if (m_engineWindowHandle)
    {
        RECT clientRect{};
        GetClientRect(m_engineWindowHandle, &clientRect);
        initialBounds = ComputeDockedBounds(clientRect);
    }

    return CreateChildWindow(initialBounds);
}

bool WebDialogHost::CreateChildWindow(const RECT& bounds)
{
    m_isFloatingWindow = false;
    m_lastBounds = bounds;
    m_hostWindowHandle = CreateWindowExW(
        0,
        kWebDialogWindowClassName,
        m_title.c_str(),
        WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
        bounds.left,
        bounds.top,
        std::max<LONG>(0, bounds.right - bounds.left),
        std::max<LONG>(0, bounds.bottom - bounds.top),
        m_engineWindowHandle,
        nullptr,
        GetModuleHandleW(nullptr),
        this);

    if (m_hostWindowHandle)
    {
        ApplyCurrentVisibility();
    }

    return m_hostWindowHandle != nullptr;
}

bool WebDialogHost::CreateFloatingWindow(const WebDialogRect& bounds)
{
    m_isFloatingWindow = true;
    m_lastBounds = ToRect(bounds);
    m_hostWindowHandle = CreateWindowExW(
        m_alwaysOnTop ? WS_EX_TOPMOST : 0,
        kWebDialogWindowClassName,
        m_title.c_str(),
        BuildFloatingWindowStyle(),
        bounds.x,
        bounds.y,
        bounds.width,
        bounds.height,
        nullptr,
        nullptr,
        GetModuleHandleW(nullptr),
        this);

    if (!m_hostWindowHandle)
    {
        return false;
    }

    ApplyCurrentVisibility();
    return true;
}

void WebDialogHost::DestroyHostWindow()
{
    if (m_hostWindowHandle)
    {
        DestroyWindow(m_hostWindowHandle);
        m_hostWindowHandle = nullptr;
    }
}

void WebDialogHost::ReleaseWebViewObjects()
{
    m_ready = false;

    if (m_controller)
    {
        m_controller->Close();
    }

    SafeRelease(m_webView);
    SafeRelease(m_controller);
    SafeRelease(m_environment);
}

void WebDialogHost::ResetInitializationState()
{
    ReleaseWebViewObjects();
    DestroyHostWindow();
    m_initialized = false;
    m_open = false;
}

void WebDialogHost::UpdateControllerBounds()
{
    if (!m_controller || !m_hostWindowHandle)
    {
        return;
    }

    RECT clientRect{};
    GetClientRect(m_hostWindowHandle, &clientRect);
    if (FAILED(m_controller->put_Bounds(clientRect)))
    {
        ShowFallbackText(L"Failed to resize the web dialog.");
        return;
    }

    m_controller->NotifyParentWindowPositionChanged();
}

void WebDialogHost::ApplyCurrentVisibility()
{
    if (!m_hostWindowHandle)
    {
        return;
    }

    ShowWindow(m_hostWindowHandle, m_visible ? SW_SHOW : SW_HIDE);
}

RECT WebDialogHost::ComputeDockedBounds(const RECT& engineClientRect) const
{
    const long clientWidth = std::max<LONG>(0, engineClientRect.right - engineClientRect.left);
    const long clientHeight = std::max<LONG>(0, engineClientRect.bottom - engineClientRect.top);
    const long dockSize = std::max<long>(1, static_cast<long>(m_dockSize));

    switch (m_dockMode)
    {
    case WebDialogDockMode::Left:
        return RECT{ 0, 0, std::min<long>(dockSize, clientWidth), clientHeight };

    case WebDialogDockMode::Right:
        return RECT{
            std::max<long>(0, clientWidth - std::min<long>(dockSize, clientWidth)),
            0,
            clientWidth,
            clientHeight
        };

    case WebDialogDockMode::Top:
        return RECT{ 0, 0, clientWidth, std::min<long>(dockSize, clientHeight) };

    case WebDialogDockMode::Bottom:
        return RECT{
            0,
            std::max<long>(0, clientHeight - std::min<long>(dockSize, clientHeight)),
            clientWidth,
            clientHeight
        };

    case WebDialogDockMode::Fill:
        return RECT{ 0, 0, clientWidth, clientHeight };

    case WebDialogDockMode::ManualChild:
        return ToRect(m_bounds);

    case WebDialogDockMode::Floating:
    default:
        return ToRect(m_bounds);
    }
}

RECT WebDialogHost::ToRect(const WebDialogRect& bounds) const
{
    RECT rect{};
    rect.left = bounds.x;
    rect.top = bounds.y;
    rect.right = bounds.x + std::max(0, bounds.width);
    rect.bottom = bounds.y + std::max(0, bounds.height);
    return rect;
}

WebDialogRect WebDialogHost::ToDialogRect(const RECT& bounds) const
{
    WebDialogRect rect;
    rect.x = bounds.left;
    rect.y = bounds.top;
    rect.width = std::max<LONG>(0, bounds.right - bounds.left);
    rect.height = std::max<LONG>(0, bounds.bottom - bounds.top);
    return rect;
}

std::wstring WebDialogHost::BuildContentUri() const
{
    if (m_contentPath.empty())
    {
        return std::wstring();
    }

    std::wstring absolutePath = ToWideString(m_contentPath);
    std::replace(absolutePath.begin(), absolutePath.end(), L'\\', L'/');

    if (absolutePath.size() >= 2 && absolutePath[1] == L':')
    {
        absolutePath.insert(absolutePath.begin(), L'/');
    }

    return L"file://" + EncodeFilePathSegment(absolutePath);
}

void WebDialogHost::ShowFallbackText(const wchar_t* text)
{
    if (m_hostWindowHandle)
    {
        SetWindowTextW(m_hostWindowHandle, text ? text : L"Web dialog is unavailable.");
    }
}

bool WebDialogHost::NavigateToCurrentContent()
{
    if (!m_webView)
    {
        return false;
    }

    const std::wstring contentUri = BuildContentUri();
    if (contentUri.empty())
    {
        ShowFallbackText(L"Web dialog content path is invalid.");
        return false;
    }

    const std::wstring contentPath = ToWideString(m_contentPath);
    if (!FileExists(contentPath))
    {
        ShowFallbackText(L"Web dialog content file was not found.");
        return false;
    }

    if (FAILED(m_webView->Navigate(contentUri.c_str())))
    {
        ShowFallbackText(L"Failed to navigate the web dialog.");
        return false;
    }

    return true;
}

bool WebDialogHost::RecreateHostWindow()
{
    if (!m_initialized)
    {
        return false;
    }

    ReleaseWebViewObjects();
    DestroyHostWindow();
    m_ready = false;
    m_open = false;

    if (!CreateHostWindow())
    {
        return false;
    }

    auto* handler = new EnvironmentCompletedHandler(this);
    const auto createEnvironment = GetCreateEnvironmentFunction(m_webViewLoaderModule);
    const HRESULT result = createEnvironment ? createEnvironment(nullptr, nullptr, nullptr, handler) : E_FAIL;
    handler->Release();

    if (FAILED(result))
    {
        ShowFallbackText(L"Failed to restart the WebView2 environment.");
        ResetInitializationState();
        return false;
    }

    m_open = true;
    return true;
}

DWORD WebDialogHost::BuildFloatingWindowStyle() const
{
    DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
    if (m_resizable)
    {
        style |= WS_THICKFRAME | WS_MAXIMIZEBOX;
    }

    return style;
}

bool WebDialogHost::EnsureWindowClassRegistered()
{
    static bool registered = false;
    if (registered)
    {
        return true;
    }

    WNDCLASSEXW windowClass{};
    windowClass.cbSize = sizeof(WNDCLASSEXW);
    windowClass.style = CS_HREDRAW | CS_VREDRAW;
    windowClass.lpfnWndProc = WindowProcSetup;
    windowClass.hInstance = GetModuleHandleW(nullptr);
    windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
    windowClass.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    windowClass.lpszClassName = kWebDialogWindowClassName;

    if (!RegisterClassExW(&windowClass))
    {
        const DWORD error = GetLastError();
        if (error != ERROR_CLASS_ALREADY_EXISTS)
        {
            return false;
        }
    }

    registered = true;
    return true;
}

LRESULT CALLBACK WebDialogHost::WindowProcSetup(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (msg == WM_NCCREATE)
    {
        CREATESTRUCTW* createStruct = reinterpret_cast<CREATESTRUCTW*>(lParam);
        WebDialogHost* host = reinterpret_cast<WebDialogHost*>(createStruct->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(host));
        SetWindowLongPtrW(hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(&WebDialogHost::WindowProcThunk));
        return host ? host->HandleMessage(hwnd, msg, wParam, lParam) : DefWindowProcW(hwnd, msg, wParam, lParam);
    }

    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

LRESULT CALLBACK WebDialogHost::WindowProcThunk(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    WebDialogHost* host = reinterpret_cast<WebDialogHost*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    return host ? host->HandleMessage(hwnd, msg, wParam, lParam) : DefWindowProcW(hwnd, msg, wParam, lParam);
}

LRESULT WebDialogHost::HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_NCCREATE:
        return DefWindowProcW(hwnd, msg, wParam, lParam);

    case WM_CLOSE:
        if (m_isFloatingWindow)
        {
            ShowWindow(hwnd, SW_HIDE);
            m_visible = false;
            return 0;
        }
        return DefWindowProcW(hwnd, msg, wParam, lParam);

    case WM_DESTROY:
        if (hwnd == m_hostWindowHandle)
        {
            m_hostWindowHandle = nullptr;
            m_open = false;
            m_ready = false;
        }
        return 0;
    }

    return DefWindowProcW(hwnd, msg, wParam, lParam);
}
