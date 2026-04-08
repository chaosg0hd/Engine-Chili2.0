#pragma once

#include <windows.h>

#include <cstdint>
#include <string>
#include <vector>

#ifdef IsMaximized
#undef IsMaximized
#endif

#ifdef IsMinimized
#undef IsMinimized
#endif

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
    bool IsMaximized() const;
    bool IsMinimized() const;
    HWND GetHandle() const;
    void* GetNativeHandle() const;
    int GetClientWidth() const;
    int GetClientHeight() const;
    std::uint32_t GetWidth() const;
    std::uint32_t GetHeight() const;
    float GetClientAspectRatio() const;

    const std::vector<Event>& GetEvents() const;
    void ClearEvents();

    void SetOverlayText(const std::wstring& text);
    const std::wstring& GetOverlayText() const;
    void SetTitle(const std::wstring& title);
    std::wstring GetTitle() const;
    void Maximize();
    void Restore();
    void Minimize();
    void SetClientSize(int width, int height);
    void SetCursorVisible(bool visible);
    bool IsCursorVisible() const;
    void SetCursorLocked(bool locked);
    bool IsCursorLocked() const;

private:
    static LRESULT CALLBACK WindowProcSetup(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK WindowProcThunk(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    LRESULT HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    void DrawOverlayText(HDC dc);
    void UpdateCursorClip();

private:
    HINSTANCE m_instance;
    HWND m_hwnd;
    bool m_isOpen;
    bool m_isActive;
    bool m_isCursorVisible;
    bool m_isCursorLocked;
    std::vector<Event> m_events;
    std::wstring m_overlayText;
};
