#include "platform_window.hpp"

#include <windowsx.h>
#include <algorithm>

#ifdef IsMaximized
#undef IsMaximized
#endif

#ifdef IsMinimized
#undef IsMinimized
#endif

PlatformWindow::PlatformWindow()
    : m_instance(GetModuleHandleW(nullptr)),
      m_hwnd(nullptr),
      m_isOpen(false),
      m_isActive(false),
      m_isCursorVisible(true),
      m_isCursorLocked(false),
      m_overlayText(L"Project Engine")
{
}

PlatformWindow::~PlatformWindow()
{
    if (m_hwnd != nullptr)
    {
        DestroyWindow(m_hwnd);
        m_hwnd = nullptr;
    }
}

bool PlatformWindow::Create(const wchar_t* title, int clientWidth, int clientHeight)
{
    const wchar_t* className = L"ProjectEngineWindowClass";

    WNDCLASSEXW windowClass = {};
    windowClass.cbSize = sizeof(WNDCLASSEXW);
    windowClass.style = CS_HREDRAW | CS_VREDRAW;
    windowClass.lpfnWndProc = PlatformWindow::WindowProcSetup;
    windowClass.cbClsExtra = 0;
    windowClass.cbWndExtra = 0;
    windowClass.hInstance = m_instance;
    windowClass.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
    windowClass.hbrBackground = nullptr;
    windowClass.lpszMenuName = nullptr;
    windowClass.lpszClassName = className;
    windowClass.hIconSm = LoadIcon(nullptr, IDI_APPLICATION);

    if (!RegisterClassExW(&windowClass))
    {
        const DWORD error = GetLastError();

        if (error != ERROR_CLASS_ALREADY_EXISTS)
        {
            return false;
        }
    }

    RECT windowRect = { 0, 0, clientWidth, clientHeight };

    if (!AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE))
    {
        return false;
    }

    const int windowWidth = windowRect.right - windowRect.left;
    const int windowHeight = windowRect.bottom - windowRect.top;

    m_hwnd = CreateWindowExW(
        0,
        className,
        title,
        WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        windowWidth,
        windowHeight,
        nullptr,
        nullptr,
        m_instance,
        this
    );

    if (m_hwnd == nullptr)
    {
        return false;
    }

    m_isOpen = true;
    return true;
}

void PlatformWindow::Show(int cmdShow)
{
    if (m_hwnd == nullptr)
    {
        return;
    }

    ShowWindow(m_hwnd, cmdShow);
    UpdateWindow(m_hwnd);
}

void PlatformWindow::PollEvents()
{
    MSG msg = {};

    while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
}

bool PlatformWindow::IsOpen() const
{
    return m_isOpen;
}

bool PlatformWindow::IsActive() const
{
    return m_isActive;
}

bool PlatformWindow::IsMaximized() const
{
    return (m_hwnd != nullptr) ? (IsZoomed(m_hwnd) != FALSE) : false;
}

bool PlatformWindow::IsMinimized() const
{
    return (m_hwnd != nullptr) ? (IsIconic(m_hwnd) != FALSE) : false;
}

HWND PlatformWindow::GetHandle() const
{
    return m_hwnd;
}

void* PlatformWindow::GetNativeHandle() const
{
    return m_hwnd;
}

int PlatformWindow::GetClientWidth() const
{
    if (m_hwnd == nullptr)
    {
        return 0;
    }

    RECT clientRect = {};
    if (!GetClientRect(m_hwnd, &clientRect))
    {
        return 0;
    }

    return clientRect.right - clientRect.left;
}

int PlatformWindow::GetClientHeight() const
{
    if (m_hwnd == nullptr)
    {
        return 0;
    }

    RECT clientRect = {};
    if (!GetClientRect(m_hwnd, &clientRect))
    {
        return 0;
    }

    return clientRect.bottom - clientRect.top;
}

std::uint32_t PlatformWindow::GetWidth() const
{
    return static_cast<std::uint32_t>(std::max(0, GetClientWidth()));
}

std::uint32_t PlatformWindow::GetHeight() const
{
    return static_cast<std::uint32_t>(std::max(0, GetClientHeight()));
}

float PlatformWindow::GetClientAspectRatio() const
{
    const int width = GetClientWidth();
    const int height = GetClientHeight();

    if (height <= 0)
    {
        return 0.0f;
    }

    return static_cast<float>(width) / static_cast<float>(height);
}

const std::vector<PlatformWindow::Event>& PlatformWindow::GetEvents() const
{
    return m_events;
}

void PlatformWindow::ClearEvents()
{
    m_events.clear();
}

void PlatformWindow::SetOverlayText(const std::wstring& text)
{
    m_overlayText = text;

    if (m_hwnd != nullptr)
    {
        InvalidateRect(m_hwnd, nullptr, FALSE);
    }
}

const std::wstring& PlatformWindow::GetOverlayText() const
{
    return m_overlayText;
}

void PlatformWindow::SetTitle(const std::wstring& title)
{
    if (m_hwnd == nullptr)
    {
        return;
    }

    SetWindowTextW(m_hwnd, title.c_str());
}

std::wstring PlatformWindow::GetTitle() const
{
    if (m_hwnd == nullptr)
    {
        return std::wstring();
    }

    const int length = GetWindowTextLengthW(m_hwnd);
    if (length <= 0)
    {
        return std::wstring();
    }

    std::vector<wchar_t> buffer(static_cast<std::size_t>(length) + 1U, L'\0');
    GetWindowTextW(m_hwnd, buffer.data(), length + 1);
    return std::wstring(buffer.data());
}

void PlatformWindow::Maximize()
{
    if (m_hwnd == nullptr)
    {
        return;
    }

    ShowWindow(m_hwnd, SW_MAXIMIZE);
}

void PlatformWindow::Restore()
{
    if (m_hwnd == nullptr)
    {
        return;
    }

    ShowWindow(m_hwnd, SW_RESTORE);
}

void PlatformWindow::Minimize()
{
    if (m_hwnd == nullptr)
    {
        return;
    }

    ShowWindow(m_hwnd, SW_MINIMIZE);
}

void PlatformWindow::SetClientSize(int width, int height)
{
    if (m_hwnd == nullptr || width <= 0 || height <= 0)
    {
        return;
    }

    RECT windowRect = { 0, 0, width, height };
    const DWORD style = static_cast<DWORD>(GetWindowLongPtrW(m_hwnd, GWL_STYLE));
    const DWORD exStyle = static_cast<DWORD>(GetWindowLongPtrW(m_hwnd, GWL_EXSTYLE));

    if (!AdjustWindowRectEx(&windowRect, style, FALSE, exStyle))
    {
        return;
    }

    const int windowWidth = windowRect.right - windowRect.left;
    const int windowHeight = windowRect.bottom - windowRect.top;

    SetWindowPos(
        m_hwnd,
        nullptr,
        0,
        0,
        windowWidth,
        windowHeight,
        SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE
    );

    UpdateCursorClip();
}

void PlatformWindow::SetCursorVisible(bool visible)
{
    if (m_isCursorVisible == visible)
    {
        return;
    }

    if (visible)
    {
        while (ShowCursor(TRUE) < 0)
        {
        }
    }
    else
    {
        while (ShowCursor(FALSE) >= 0)
        {
        }
    }

    m_isCursorVisible = visible;
}

bool PlatformWindow::IsCursorVisible() const
{
    return m_isCursorVisible;
}

void PlatformWindow::SetCursorLocked(bool locked)
{
    m_isCursorLocked = locked;
    UpdateCursorClip();
}

bool PlatformWindow::IsCursorLocked() const
{
    return m_isCursorLocked;
}

LRESULT CALLBACK PlatformWindow::WindowProcSetup(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (msg == WM_NCCREATE)
    {
        CREATESTRUCTW* createStruct = reinterpret_cast<CREATESTRUCTW*>(lParam);
        PlatformWindow* window = reinterpret_cast<PlatformWindow*>(createStruct->lpCreateParams);

        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(window));
        SetWindowLongPtrW(hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(&PlatformWindow::WindowProcThunk));

        return window->HandleMessage(hwnd, msg, wParam, lParam);
    }

    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

LRESULT CALLBACK PlatformWindow::WindowProcThunk(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    PlatformWindow* window = reinterpret_cast<PlatformWindow*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));

    if (window != nullptr)
    {
        return window->HandleMessage(hwnd, msg, wParam, lParam);
    }

    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

LRESULT PlatformWindow::HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_NCCREATE:
        return DefWindowProcW(hwnd, msg, wParam, lParam);

    case WM_ACTIVATE:
        m_isActive = (LOWORD(wParam) != WA_INACTIVE);
        if (!m_isActive && m_isCursorLocked)
        {
            ClipCursor(nullptr);
        }
        else if (m_isActive && m_isCursorLocked)
        {
            UpdateCursorClip();
        }
        m_events.push_back({
            m_isActive ? Event::WindowActivated : Event::WindowDeactivated,
            static_cast<int>(LOWORD(wParam)),
            0
        });
        return 0;

    case WM_KEYDOWN:
        m_events.push_back({
            Event::KeyDown,
            static_cast<int>(wParam),
            static_cast<int>(lParam)
        });
        return 0;

    case WM_KEYUP:
        m_events.push_back({
            Event::KeyUp,
            static_cast<int>(wParam),
            static_cast<int>(lParam)
        });
        return 0;

    case WM_MOUSEMOVE:
        m_events.push_back({
            Event::MouseMove,
            GET_X_LPARAM(lParam),
            GET_Y_LPARAM(lParam)
        });
        return 0;

    case WM_LBUTTONDOWN:
        SetCapture(hwnd);
        m_events.push_back({ Event::MouseButtonDown, MouseLeft, 0 });
        return 0;

    case WM_LBUTTONUP:
        ReleaseCapture();
        m_events.push_back({ Event::MouseButtonUp, MouseLeft, 0 });
        return 0;

    case WM_RBUTTONDOWN:
        SetCapture(hwnd);
        m_events.push_back({ Event::MouseButtonDown, MouseRight, 0 });
        return 0;

    case WM_RBUTTONUP:
        ReleaseCapture();
        m_events.push_back({ Event::MouseButtonUp, MouseRight, 0 });
        return 0;

    case WM_MBUTTONDOWN:
        SetCapture(hwnd);
        m_events.push_back({ Event::MouseButtonDown, MouseMiddle, 0 });
        return 0;

    case WM_MBUTTONUP:
        ReleaseCapture();
        m_events.push_back({ Event::MouseButtonUp, MouseMiddle, 0 });
        return 0;

    case WM_MOUSEWHEEL:
        m_events.push_back({
            Event::MouseWheel,
            GET_WHEEL_DELTA_WPARAM(wParam),
            0
        });
        return 0;

    case WM_ERASEBKGND:
        return 1;

    case WM_PAINT:
    {
        PAINTSTRUCT paint = {};
        HDC dc = BeginPaint(hwnd, &paint);
        DrawOverlayText(dc);
        EndPaint(hwnd, &paint);
        return 0;
    }

    case WM_CLOSE:
        m_isOpen = false;
        m_events.push_back({ Event::WindowClosed, 0, 0 });
        DestroyWindow(hwnd);
        return 0;

    case WM_DESTROY:
        m_hwnd = nullptr;
        m_isOpen = false;
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

void PlatformWindow::DrawOverlayText(HDC dc)
{
    RECT clientRect = {};
    GetClientRect(m_hwnd, &clientRect);

    if (m_overlayText.empty())
    {
        return;
    }

    const int clientWidth = clientRect.right - clientRect.left;
    const int clientHeight = clientRect.bottom - clientRect.top;
    if (clientWidth <= 0 || clientHeight <= 0)
    {
        return;
    }

    HDC memoryDc = CreateCompatibleDC(dc);
    if (memoryDc == nullptr)
    {
        return;
    }

    HBITMAP bitmap = CreateCompatibleBitmap(dc, clientWidth, clientHeight);
    if (bitmap == nullptr)
    {
        DeleteDC(memoryDc);
        return;
    }

    HGDIOBJ oldBitmap = SelectObject(memoryDc, bitmap);
    HBRUSH backgroundBrush = CreateSolidBrush(RGB(0, 0, 0));
    FillRect(memoryDc, &clientRect, backgroundBrush);
    DeleteObject(backgroundBrush);

    RECT textRect = clientRect;
    textRect.left += 12;
    textRect.top += 10;
    textRect.right -= 12;
    textRect.bottom -= 12;

    SetBkMode(memoryDc, TRANSPARENT);
    SetTextColor(memoryDc, RGB(255, 255, 255));

    DrawTextW(
        memoryDc,
        m_overlayText.c_str(),
        -1,
        &textRect,
        DT_LEFT | DT_TOP | DT_NOPREFIX | DT_WORDBREAK
    );

    BitBlt(dc, 0, 0, clientWidth, clientHeight, memoryDc, 0, 0, SRCCOPY);

    SelectObject(memoryDc, oldBitmap);
    DeleteObject(bitmap);
    DeleteDC(memoryDc);
}

void PlatformWindow::UpdateCursorClip()
{
    if (!m_isCursorLocked || m_hwnd == nullptr || !m_isActive)
    {
        ClipCursor(nullptr);
        return;
    }

    RECT clientRect = {};
    if (!GetClientRect(m_hwnd, &clientRect))
    {
        ClipCursor(nullptr);
        return;
    }

    POINT topLeft = { clientRect.left, clientRect.top };
    POINT bottomRight = { clientRect.right, clientRect.bottom };

    ClientToScreen(m_hwnd, &topLeft);
    ClientToScreen(m_hwnd, &bottomRight);

    RECT clipRect = { topLeft.x, topLeft.y, bottomRight.x, bottomRight.y };
    ClipCursor(&clipRect);
}
