#pragma once

#include "app/app_capabilities.hpp"

#include <string>

namespace studio
{
    class StudioConsolePanel
    {
    public:
        bool Open(AppCapabilities& capabilities, const std::string& contentPath, int dockInsetLeft = 0, int dockInsetRight = 0);
        void Close(AppCapabilities& capabilities);

    private:
        IAppUi::WebDialogHandle m_dialogHandle = 0U;
    };
}
