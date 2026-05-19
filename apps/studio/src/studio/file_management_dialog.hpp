#pragma once

#include "app/app_capabilities.hpp"

#include <string>

namespace studio
{
    class FileManagementDialog
    {
    public:
        bool Open(AppCapabilities& capabilities, const std::string& contentPath);
        void Close(AppCapabilities& capabilities);
        bool IsOpen() const;
        IAppUi::WebDialogHandle GetHandle() const;

    private:
        IAppUi::WebDialogHandle m_dialogHandle = 0U;
    };
}
