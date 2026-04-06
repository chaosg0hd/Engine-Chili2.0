#pragma once

#include <windows.h>

#include <string>
#include <vector>

class PlatformWindow
{

public:
    struct Event
    {
        enum Type
        {
            None,
            WindowClosed,
            WindowActivated,
            WindowDeactivated,
            KeyDown,
            KeyUp,
            MouseMove,
            MouseButtonDown,
            MouseButtonUp,
            MouseWheel
        };

        Type type = None;
        int a = 0;
        int b = 0;
    };

    static constexpr int MouseLeft = 0;
    static constexpr int MouseRight = 1;
    static constexpr int MouseMiddle = 2;

public:    
    PlatformWindow();
    ~PlatformWindow();

    bool Create(const wchar_t* title, int clientWidth, int clientHeight);
    void Show(int cmdShow);
    void PollEvents();

    bool IsOpen() const;
    bool IsActive() const;
    HWND GetHandle() const;
    int GetClientWidth() const;
    int GetClientHeight() const;

    const std::vector<Event>& GetEvents() const;
    void ClearEvents();

    void SetOverlayText(const std::wstring& text);
    const std::wstring& GetOverlayText() const;
    void SetTitle(const std::wstring& title);

private:
    static LRESULT CALLBACK WindowProcSetup(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK WindowProcThunk(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    LRESULT HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    void DrawOverlayText(HDC dc);

private:
    HINSTANCE m_instance;
    HWND m_hwnd;
    bool m_isOpen;
    bool m_isActive;
    std::vector<Event> m_events;
    std::wstring m_overlayText;
};
