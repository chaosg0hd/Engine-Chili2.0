#pragma once

#include <windows.h>
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
            KeyUp
        };

        Type type = None;
        int a = 0;
        int b = 0;
    };

public:    
    PlatformWindow();
    ~PlatformWindow();

    bool Create(const wchar_t* title, int clientWidth, int clientHeight);
    void Show(int cmdShow);
    void PollEvents();

    bool IsOpen() const;
    bool IsActive() const;
    HWND GetHandle() const;
    
    const std::vector<Event>& GetEvents() const;
    void ClearEvents();    

private:
    static LRESULT CALLBACK WindowProcSetup(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK WindowProcThunk(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    LRESULT HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

private:
    HINSTANCE m_instance;
    HWND m_hwnd;
    bool m_isOpen;
    bool m_isActive;
    std::vector<Event> m_events;
    
};