#include "native_button_host.hpp"

#include <windowsx.h>

#include <algorithm>

namespace
{
    constexpr wchar_t kNativeButtonWindowClassName[] = L"ProjectEngineNativeButtonClass";

    COLORREF MixColor(COLORREF a, COLORREF b, double t)
    {
        const auto clamp = [](double value)
        {
            return std::max(0.0, std::min(255.0, value));
        };

        const double inverse = 1.0 - t;
        const BYTE red = static_cast<BYTE>(clamp((GetRValue(a) * inverse) + (GetRValue(b) * t)));
        const BYTE green = static_cast<BYTE>(clamp((GetGValue(a) * inverse) + (GetGValue(b) * t)));
        const BYTE blue = static_cast<BYTE>(clamp((GetBValue(a) * inverse) + (GetBValue(b) * t)));
        return RGB(red, green, blue);
    }

    bool EnsureNativeButtonWindowClass()
    {
        WNDCLASSEXW windowClass = {};
        windowClass.cbSize = sizeof(WNDCLASSEXW);
        windowClass.style = CS_HREDRAW | CS_VREDRAW;
        windowClass.lpfnWndProc = NativeButtonHost::WindowProcSetup;
        windowClass.hInstance = GetModuleHandleW(nullptr);
        windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
        windowClass.hbrBackground = nullptr;
        windowClass.lpszClassName = kNativeButtonWindowClassName;

        if (!RegisterClassExW(&windowClass))
        {
            return GetLastError() == ERROR_CLASS_ALREADY_EXISTS;
        }

        return true;
    }
}

bool NativeButtonHost::Initialize(HWND engineWindowHandle, const NativeButtonDesc& desc)
{
    Shutdown();

    if (!engineWindowHandle || !EnsureNativeButtonWindowClass())
    {
        return false;
    }

    m_engineWindowHandle = engineWindowHandle;
    m_text = desc.text.empty() ? L"Button" : desc.text;
    m_rect = desc.rect;
    m_visible = desc.visible;
    m_enabled = desc.enabled;
    m_hovered = false;
    m_pressedVisual = false;
    m_trackingMouse = false;
    m_pressedPending = false;

    m_buttonWindowHandle = CreateWindowExW(
        0,
        kNativeButtonWindowClassName,
        m_text.c_str(),
        WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_TABSTOP | (m_visible ? WS_VISIBLE : 0),
        m_rect.x,
        m_rect.y,
        std::max(1, m_rect.width),
        std::max(1, m_rect.height),
        m_engineWindowHandle,
        nullptr,
        GetModuleHandleW(nullptr),
        this
    );

    if (!m_buttonWindowHandle)
    {
        return false;
    }

    EnableWindow(m_buttonWindowHandle, m_enabled ? TRUE : FALSE);
    return true;
}

void NativeButtonHost::Shutdown()
{
    if (m_buttonWindowHandle)
    {
        DestroyWindow(m_buttonWindowHandle);
        m_buttonWindowHandle = nullptr;
    }

    m_engineWindowHandle = nullptr;
    m_hovered = false;
    m_pressedVisual = false;
    m_trackingMouse = false;
    m_pressedPending = false;
}

bool NativeButtonHost::SetBounds(const NativeControlRect& rect)
{
    m_rect = rect;
    if (!m_buttonWindowHandle)
    {
        return false;
    }

    return SetWindowPos(
        m_buttonWindowHandle,
        nullptr,
        rect.x,
        rect.y,
        std::max(1, rect.width),
        std::max(1, rect.height),
        SWP_NOZORDER | SWP_NOACTIVATE
    ) != FALSE;
}

bool NativeButtonHost::SetText(const std::wstring& text)
{
    m_text = text.empty() ? L"Button" : text;
    if (!m_buttonWindowHandle)
    {
        return false;
    }

    SetWindowTextW(m_buttonWindowHandle, m_text.c_str());
    InvalidateRect(m_buttonWindowHandle, nullptr, FALSE);
    return true;
}

bool NativeButtonHost::SetVisible(bool visible)
{
    m_visible = visible;
    if (!m_buttonWindowHandle)
    {
        return false;
    }

    ShowWindow(m_buttonWindowHandle, visible ? SW_SHOW : SW_HIDE);
    return true;
}

bool NativeButtonHost::SetEnabled(bool enabled)
{
    m_enabled = enabled;
    if (!m_buttonWindowHandle)
    {
        return false;
    }

    EnableWindow(m_buttonWindowHandle, enabled ? TRUE : FALSE);
    InvalidateRect(m_buttonWindowHandle, nullptr, FALSE);
    return true;
}

bool NativeButtonHost::ConsumePressed()
{
    const bool wasPressed = m_pressedPending;
    m_pressedPending = false;
    return wasPressed;
}

bool NativeButtonHost::IsOpen() const
{
    return m_buttonWindowHandle != nullptr && IsWindow(m_buttonWindowHandle) != FALSE;
}

LRESULT CALLBACK NativeButtonHost::WindowProcSetup(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (msg == WM_NCCREATE)
    {
        CREATESTRUCTW* createStruct = reinterpret_cast<CREATESTRUCTW*>(lParam);
        NativeButtonHost* host = reinterpret_cast<NativeButtonHost*>(createStruct->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(host));
        SetWindowLongPtrW(hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(&NativeButtonHost::WindowProcThunk));
        return host->HandleMessage(hwnd, msg, wParam, lParam);
    }

    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

LRESULT CALLBACK NativeButtonHost::WindowProcThunk(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    NativeButtonHost* host = reinterpret_cast<NativeButtonHost*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    return host ? host->HandleMessage(hwnd, msg, wParam, lParam) : DefWindowProcW(hwnd, msg, wParam, lParam);
}

LRESULT NativeButtonHost::HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_NCCREATE:
        m_buttonWindowHandle = hwnd;
        return DefWindowProcW(hwnd, msg, wParam, lParam);

    case WM_ERASEBKGND:
        return 1;

    case WM_MOUSEMOVE:
        if (!m_hovered)
        {
            m_hovered = true;
            InvalidateRect(hwnd, nullptr, FALSE);
        }

        if (!m_trackingMouse)
        {
            TRACKMOUSEEVENT track = {};
            track.cbSize = sizeof(TRACKMOUSEEVENT);
            track.dwFlags = TME_LEAVE;
            track.hwndTrack = hwnd;
            if (TrackMouseEvent(&track))
            {
                m_trackingMouse = true;
            }
        }
        return 0;

    case WM_MOUSELEAVE:
        m_hovered = false;
        m_trackingMouse = false;
        m_pressedVisual = false;
        InvalidateRect(hwnd, nullptr, FALSE);
        return 0;

    case WM_LBUTTONDOWN:
        SetCapture(hwnd);
        SetFocus(hwnd);
        m_pressedVisual = true;
        InvalidateRect(hwnd, nullptr, FALSE);
        return 0;

    case WM_LBUTTONUP:
    {
        if (GetCapture() == hwnd)
        {
            ReleaseCapture();
        }

        RECT rect = GetClientRectSafe();
        const POINT point{ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
        if (m_pressedVisual && PtInRect(&rect, point))
        {
            m_pressedPending = true;
        }

        m_pressedVisual = false;
        InvalidateRect(hwnd, nullptr, FALSE);
        return 0;
    }

    case WM_KEYDOWN:
        if (wParam == VK_SPACE || wParam == VK_RETURN)
        {
            m_pressedVisual = true;
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        }
        break;

    case WM_KEYUP:
        if (wParam == VK_SPACE || wParam == VK_RETURN)
        {
            if (m_pressedVisual)
            {
                m_pressedPending = true;
            }
            m_pressedVisual = false;
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        }
        break;

    case WM_SETFOCUS:
    case WM_KILLFOCUS:
        if (msg == WM_KILLFOCUS)
        {
            m_pressedVisual = false;
        }
        InvalidateRect(hwnd, nullptr, FALSE);
        return 0;

    case WM_ENABLE:
        m_enabled = (wParam != FALSE);
        InvalidateRect(hwnd, nullptr, FALSE);
        return 0;

    case WM_PAINT:
    {
        PAINTSTRUCT paint = {};
        HDC dc = BeginPaint(hwnd, &paint);
        Draw(dc);
        EndPaint(hwnd, &paint);
        return 0;
    }

    case WM_DESTROY:
        if (m_buttonWindowHandle == hwnd)
        {
            m_buttonWindowHandle = nullptr;
        }
        return 0;
    }

    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

void NativeButtonHost::Draw(HDC dc)
{
    RECT clientRect = GetClientRectSafe();
    const int width = clientRect.right - clientRect.left;
    const int height = clientRect.bottom - clientRect.top;
    if (width <= 0 || height <= 0)
    {
        return;
    }

    HDC memoryDc = CreateCompatibleDC(dc);
    if (!memoryDc)
    {
        return;
    }

    HBITMAP bitmap = CreateCompatibleBitmap(dc, width, height);
    if (!bitmap)
    {
        DeleteDC(memoryDc);
        return;
    }

    HGDIOBJ oldBitmap = SelectObject(memoryDc, bitmap);

    const COLORREF baseColor = RGB(67, 64, 66);
    const COLORREF hoverColor = RGB(88, 79, 74);
    const COLORREF pressedColor = RGB(118, 92, 70);
    const COLORREF textColor = m_enabled ? RGB(255, 250, 246) : RGB(182, 172, 166);
    const COLORREF outlineColor = RGB(96, 90, 93);
    const COLORREF focusColor = RGB(255, 183, 125);

    COLORREF fillColor = baseColor;
    if (!m_enabled)
    {
        fillColor = MixColor(baseColor, RGB(20, 20, 20), 0.30);
    }
    else if (m_pressedVisual)
    {
        fillColor = pressedColor;
    }
    else if (m_hovered)
    {
        fillColor = hoverColor;
    }

    HBRUSH backgroundBrush = CreateSolidBrush(fillColor);
    HPEN outlinePen = CreatePen(PS_SOLID, 1, outlineColor);
    HGDIOBJ oldBrush = SelectObject(memoryDc, backgroundBrush);
    HGDIOBJ oldPen = SelectObject(memoryDc, outlinePen);

    RoundRect(memoryDc, clientRect.left, clientRect.top, clientRect.right, clientRect.bottom, 10, 10);

    SelectObject(memoryDc, oldPen);
    SelectObject(memoryDc, oldBrush);
    DeleteObject(outlinePen);
    DeleteObject(backgroundBrush);

    if (GetFocus() == m_buttonWindowHandle)
    {
        HPEN focusPen = CreatePen(PS_SOLID, 1, focusColor);
        HGDIOBJ oldFocusPen = SelectObject(memoryDc, focusPen);
        HGDIOBJ oldNullBrush = SelectObject(memoryDc, GetStockObject(NULL_BRUSH));
        Rectangle(memoryDc, 3, 3, width - 3, height - 3);
        SelectObject(memoryDc, oldNullBrush);
        SelectObject(memoryDc, oldFocusPen);
        DeleteObject(focusPen);
    }

    HFONT font = static_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
    HGDIOBJ oldFont = SelectObject(memoryDc, font);
    SetBkMode(memoryDc, TRANSPARENT);
    SetTextColor(memoryDc, textColor);

    RECT textRect = clientRect;
    textRect.left += 10;
    textRect.right -= 10;
    DrawTextW(
        memoryDc,
        m_text.c_str(),
        -1,
        &textRect,
        DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX
    );

    SelectObject(memoryDc, oldFont);

    BitBlt(dc, 0, 0, width, height, memoryDc, 0, 0, SRCCOPY);

    SelectObject(memoryDc, oldBitmap);
    DeleteObject(bitmap);
    DeleteDC(memoryDc);
}

RECT NativeButtonHost::GetClientRectSafe() const
{
    RECT rect{};
    if (m_buttonWindowHandle)
    {
        GetClientRect(m_buttonWindowHandle, &rect);
    }
    return rect;
}
