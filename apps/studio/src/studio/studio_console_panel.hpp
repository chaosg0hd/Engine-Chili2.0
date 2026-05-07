#pragma once

#include "app/app_capabilities.hpp"

#include <string>

namespace studio
{
    class StudioConsolePanel
    {
    public:
        bool Open(AppCapabilities& capabilities, const std::string& contentPath, int dockInsetLeft = 0, int dockInsetRight = 0, int dockHeight = 196);
        void Close(AppCapabilities& capabilities);
        bool SetVisible(AppCapabilities& capabilities, bool visible);
        bool IsVisible() const;

    private:
        IAppUi::WebDialogHandle m_dialogHandle = 0U;
        bool m_visible = false;
    };
}
