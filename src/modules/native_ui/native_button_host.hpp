#pragma once

#include "../../core/engine_core.hpp"

#include <windows.h>

#include <string>

class NativeButtonHost
{
public:
    bool Initialize(HWND engineWindowHandle, const NativeButtonDesc& desc);
    void Shutdown();

    bool SetBounds(const NativeControlRect& rect);
    bool SetText(const std::wstring& text);
    bool SetVisible(bool visible);
    bool SetEnabled(bool enabled);

    bool ConsumePressed();
    bool IsOpen() const;

    static LRESULT CALLBACK WindowProcSetup(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK WindowProcThunk(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

private:
    LRESULT HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    void Draw(HDC dc);
    RECT GetClientRectSafe() const;

private:
    HWND m_engineWindowHandle = nullptr;
    HWND m_buttonWindowHandle = nullptr;
    std::wstring m_text = L"Button";
    NativeControlRect m_rect{};
    bool m_visible = true;
    bool m_enabled = true;
    bool m_hovered = false;
    bool m_pressedVisual = false;
    bool m_trackingMouse = false;
    bool m_pressedPending = false;
};
