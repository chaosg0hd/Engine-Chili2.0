#include "dock_layout.hpp"

#include <algorithm>

DockLayout::Regions DockLayout::Compute(const RECT& clientRect) const
{
    const long clientWidth = std::max<LONG>(0, clientRect.right - clientRect.left);
    const long clientHeight = std::max<LONG>(0, clientRect.bottom - clientRect.top);

    const long requestedDockWidth = static_cast<long>(m_leftDockWidth);
    const long dockWidth = std::clamp<long>(
        requestedDockWidth,
        220,
        std::max<long>(220, clientWidth / 2));

    Regions regions;
    regions.leftDock = MakeRect(0, 0, std::min<long>(dockWidth, clientWidth), clientHeight);
    regions.mainArea = MakeRect(
        regions.leftDock.right,
        0,
        clientWidth,
        clientHeight);
    return regions;
}

int DockLayout::GetLeftDockWidth() const
{
    return m_leftDockWidth;
}

RECT DockLayout::MakeRect(long left, long top, long right, long bottom)
{
    RECT rect{};
    rect.left = left;
    rect.top = top;
    rect.right = right;
    rect.bottom = bottom;
    return rect;
}
