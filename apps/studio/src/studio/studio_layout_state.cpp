#include "studio/studio_layout_state.hpp"

#include <algorithm>

namespace studio
{
    void StudioLayoutState::SetChromeSizes(
        int topToolbarHeight,
        int leftSidebarWidth,
        int rightInspectorWidth,
        int bottomConsoleHeight)
    {
        m_topToolbarHeight = std::max(0, topToolbarHeight);
        m_leftSidebarWidth = std::max(0, leftSidebarWidth);
        m_rightInspectorWidth = std::max(0, rightInspectorWidth);
        m_bottomConsoleHeight = std::max(0, bottomConsoleHeight);
    }

    void StudioLayoutState::Recalculate(
        int windowWidth,
        int windowHeight,
        bool rightInspectorVisible,
        bool bottomConsoleVisible)
    {
        m_windowWidth = std::max(0, windowWidth);
        m_windowHeight = std::max(0, windowHeight);

        const int rightInset = rightInspectorVisible ? m_rightInspectorWidth : 0;
        const int bottomInset = bottomConsoleVisible ? m_bottomConsoleHeight : 0;
        m_viewportRect.x = m_leftSidebarWidth;
        m_viewportRect.y = m_topToolbarHeight;
        m_viewportRect.width = std::max(1, m_windowWidth - m_leftSidebarWidth - rightInset);
        m_viewportRect.height = std::max(1, m_windowHeight - m_topToolbarHeight - bottomInset);
    }

    int StudioLayoutState::GetWindowWidth() const
    {
        return m_windowWidth;
    }

    int StudioLayoutState::GetWindowHeight() const
    {
        return m_windowHeight;
    }

    int StudioLayoutState::GetTopToolbarHeight() const
    {
        return m_topToolbarHeight;
    }

    int StudioLayoutState::GetLeftSidebarWidth() const
    {
        return m_leftSidebarWidth;
    }

    int StudioLayoutState::GetRightInspectorWidth() const
    {
        return m_rightInspectorWidth;
    }

    int StudioLayoutState::GetBottomConsoleHeight() const
    {
        return m_bottomConsoleHeight;
    }

    const ViewportRect& StudioLayoutState::GetViewportRect() const
    {
        return m_viewportRect;
    }
}
