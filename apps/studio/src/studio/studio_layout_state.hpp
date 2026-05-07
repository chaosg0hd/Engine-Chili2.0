#pragma once

#include "modules/render/render_types.hpp"

namespace studio
{
    class StudioLayoutState
    {
    public:
        void SetChromeSizes(int topToolbarHeight, int leftSidebarWidth, int rightInspectorWidth, int bottomConsoleHeight);
        void Recalculate(int windowWidth, int windowHeight, bool rightInspectorVisible, bool bottomConsoleVisible);

        int GetWindowWidth() const;
        int GetWindowHeight() const;
        int GetTopToolbarHeight() const;
        int GetLeftSidebarWidth() const;
        int GetRightInspectorWidth() const;
        int GetBottomConsoleHeight() const;
        const ViewportRect& GetViewportRect() const;

    private:
        int m_windowWidth = 0;
        int m_windowHeight = 0;
        int m_topToolbarHeight = 104;
        int m_leftSidebarWidth = 128;
        int m_rightInspectorWidth = 300;
        int m_bottomConsoleHeight = 196;
        ViewportRect m_viewportRect{ 0, 0, 1, 1 };
    };
}
