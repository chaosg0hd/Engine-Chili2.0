#include "platform_window.hpp"

PlatformWindow::PlatformWindow()
    : m_instance(GetModuleHandleW(nullptr)),
      m_hwnd(nullptr),
      m_isOpen(false),
      m_isActive(false),
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
    windowClass.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
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
        WS_OVERLAPPEDWINDOW,
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

HWND PlatformWindow::GetHandle() const
{
    return m_hwnd;
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
        InvalidateRect(m_hwnd, nullptr, TRUE);
    }
}

const std::wstring& PlatformWindow::GetOverlayText() const
{
    return m_overlayText;
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

    RECT textRect = clientRect;
    textRect.left += 12;
    textRect.top += 10;
    textRect.right -= 12;
    textRect.bottom = textRect.top + 80;

    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, RGB(20, 20, 20));

    DrawTextW(
        dc,
        m_overlayText.c_str(),
        -1,
        &textRect,
        DT_LEFT | DT_TOP | DT_NOPREFIX | DT_WORDBREAK
    );
}
