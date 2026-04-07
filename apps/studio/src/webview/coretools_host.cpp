#include "coretools_host.hpp"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif

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
    using CreateCoreWebView2EnvironmentWithOptionsFn = HRESULT(STDAPICALLTYPE*)(
        PCWSTR browserExecutableFolder,
        PCWSTR userDataFolder,
        IUnknown* environmentOptions,
        ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler* environmentCreatedHandler);

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
        explicit EnvironmentCompletedHandler(CoreToolsHost* owner)
            : m_owner(owner)
        {
        }

        HRESULT STDMETHODCALLTYPE QueryInterface(REFIID, void** object) override
        {
            if (!object)
            {
                return E_POINTER;
            }

            *object = nullptr;
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
        CoreToolsHost* m_owner = nullptr;
        long m_referenceCount = 1;
    };

    class ControllerCompletedHandler final : public ICoreWebView2CreateCoreWebView2ControllerCompletedHandler
    {
    public:
        explicit ControllerCompletedHandler(CoreToolsHost* owner)
            : m_owner(owner)
        {
        }

        HRESULT STDMETHODCALLTYPE QueryInterface(REFIID, void** object) override
        {
            if (!object)
            {
                return E_POINTER;
            }

            *object = nullptr;
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
        CoreToolsHost* m_owner = nullptr;
        long m_referenceCount = 1;
    };

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
}

bool CoreToolsHost::Initialize(HWND parentWindowHandle, const RECT& initialBounds, const std::string& contentPath)
{
    if (m_initialized)
    {
        return true;
    }

    if (!parentWindowHandle)
    {
        return false;
    }

    m_parentWindowHandle = parentWindowHandle;
    m_bounds = initialBounds;
    m_contentPath = contentPath;
    m_ready = false;

    if (!EnsureComApartment())
    {
        ShowFallbackText(L"Failed to initialize COM for CoreTools.");
        return false;
    }

    if (!CreateHostWindow())
    {
        return false;
    }

    if (!LoadWebViewLoader())
    {
        ShowFallbackText(L"WebView2 loader not found.\r\nCopy WebView2Loader.dll beside engine_studio.exe.");
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
        ShowFallbackText(L"Failed to start the CoreTools WebView2 environment.");
        ResetInitializationState();
        return false;
    }

    m_initialized = true;
    return true;
}

void CoreToolsHost::Resize(const RECT& bounds)
{
    m_bounds = bounds;

    if (m_hostWindowHandle)
    {
        SetWindowPos(
            m_hostWindowHandle,
            nullptr,
            m_bounds.left,
            m_bounds.top,
            std::max<LONG>(0, m_bounds.right - m_bounds.left),
            std::max<LONG>(0, m_bounds.bottom - m_bounds.top),
            SWP_NOACTIVATE | SWP_NOZORDER);
    }

    UpdateControllerBounds();
}

void CoreToolsHost::Shutdown()
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

    m_parentWindowHandle = nullptr;
    m_contentPath.clear();
    ZeroMemory(&m_bounds, sizeof(m_bounds));
    m_initialized = false;
    m_ready = false;
}

bool CoreToolsHost::IsInitialized() const
{
    return m_initialized;
}

bool CoreToolsHost::IsReady() const
{
    return m_ready;
}

void CoreToolsHost::HandleEnvironmentCreated(HRESULT result, ICoreWebView2Environment* environment)
{
    m_ready = false;

    if (FAILED(result) || !environment)
    {
        ShowFallbackText(L"Failed to create the CoreTools WebView2 environment.");
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
        ShowFallbackText(L"Failed to create the CoreTools WebView2 controller.");
        ResetInitializationState();
    }
}

void CoreToolsHost::HandleControllerCreated(HRESULT result, ICoreWebView2Controller* controller)
{
    m_ready = false;

    if (FAILED(result) || !controller)
    {
        ShowFallbackText(L"Failed to attach CoreTools to the studio window.");
        ResetInitializationState();
        return;
    }

    SafeRelease(m_webView);
    SafeRelease(m_controller);
    m_controller = controller;
    m_controller->AddRef();

    if (FAILED(m_controller->put_IsVisible(TRUE)))
    {
        ShowFallbackText(L"Failed to make the CoreTools WebView visible.");
        ResetInitializationState();
        return;
    }

    UpdateControllerBounds();

    ICoreWebView2* webView = nullptr;
    if (FAILED(m_controller->get_CoreWebView2(&webView)) || !webView)
    {
        ShowFallbackText(L"Failed to access the CoreTools WebView.");
        ResetInitializationState();
        return;
    }

    m_webView = webView;

    const std::wstring contentUri = BuildContentUri();
    if (contentUri.empty())
    {
        ShowFallbackText(L"CoreTools content path is invalid.");
        ResetInitializationState();
        return;
    }

    const HRESULT navigationResult = m_webView->Navigate(contentUri.c_str());
    if (FAILED(navigationResult))
    {
        ShowFallbackText(L"Failed to navigate the embedded CoreTools surface.");
        ResetInitializationState();
        return;
    }

    m_ready = true;
}

bool CoreToolsHost::EnsureComApartment()
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

bool CoreToolsHost::CreateHostWindow()
{
    if (m_hostWindowHandle)
    {
        return true;
    }

    m_hostWindowHandle = CreateWindowExW(
        0,
        L"STATIC",
        L"Loading CoreTools...",
        WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
        m_bounds.left,
        m_bounds.top,
        std::max<LONG>(0, m_bounds.right - m_bounds.left),
        std::max<LONG>(0, m_bounds.bottom - m_bounds.top),
        m_parentWindowHandle,
        nullptr,
        GetModuleHandleW(nullptr),
        nullptr);

    return m_hostWindowHandle != nullptr;
}

bool CoreToolsHost::LoadWebViewLoader()
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

void CoreToolsHost::DestroyHostWindow()
{
    if (m_hostWindowHandle)
    {
        DestroyWindow(m_hostWindowHandle);
        m_hostWindowHandle = nullptr;
    }
}

void CoreToolsHost::ReleaseWebViewObjects()
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

void CoreToolsHost::ResetInitializationState()
{
    ReleaseWebViewObjects();

    if (m_webViewLoaderModule)
    {
        FreeLibrary(m_webViewLoaderModule);
        m_webViewLoaderModule = nullptr;
    }

    m_initialized = false;
}

void CoreToolsHost::UpdateControllerBounds()
{
    if (!m_controller || !m_hostWindowHandle)
    {
        return;
    }

    RECT clientRect{};
    GetClientRect(m_hostWindowHandle, &clientRect);
    if (FAILED(m_controller->put_Bounds(clientRect)))
    {
        ShowFallbackText(L"Failed to resize the CoreTools surface.");
        return;
    }

    m_controller->NotifyParentWindowPositionChanged();
}

std::wstring CoreToolsHost::BuildContentUri() const
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

void CoreToolsHost::ShowFallbackText(const wchar_t* text)
{
    if (m_hostWindowHandle)
    {
        SetWindowTextW(m_hostWindowHandle, text ? text : L"CoreTools is unavailable.");
    }
}
