#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>

class DockLayout
{
public:
    struct Regions
    {
        RECT leftDock{};
        RECT mainArea{};
    };

public:
    Regions Compute(const RECT& clientRect) const;
    int GetLeftDockWidth() const;

private:
    static RECT MakeRect(long left, long top, long right, long bottom);

private:
    int m_leftDockWidth = 360;
};
