#pragma once

#include "app/app_capabilities.hpp"

#include <string>

namespace studio
{
    class SceneSettingsDialog
    {
    public:
        bool Open(AppCapabilities& capabilities, const std::string& contentPath)
        {
            if (!capabilities.ui)
            {
                return false;
            }

            if (m_dialogHandle != 0U)
            {
                capabilities.ui->SetWebDialogVisible(m_dialogHandle, true);
                return true;
            }

            WebDialogDesc dialogDesc;
            dialogDesc.name = "SceneSettingsDialog";
            dialogDesc.title = L"Scene Settings";
            dialogDesc.contentPath = contentPath;
            dialogDesc.dockMode = WebDialogDockMode::Floating;
            dialogDesc.rect = { 200, 140, 480, 480 };
            dialogDesc.visible = true;
            dialogDesc.resizable = true;
            dialogDesc.alwaysOnTop = false;

            m_dialogHandle = capabilities.ui->CreateWebDialog(dialogDesc);
            return m_dialogHandle != 0U;
        }

        void Close(AppCapabilities& capabilities)
        {
            if (m_dialogHandle == 0U || !capabilities.ui)
            {
                return;
            }

            capabilities.ui->DestroyWebDialog(m_dialogHandle);
            m_dialogHandle = 0U;
        }

        bool IsOpen() const { return m_dialogHandle != 0U; }
        IAppUi::WebDialogHandle GetHandle() const { return m_dialogHandle; }

    private:
        IAppUi::WebDialogHandle m_dialogHandle = 0U;
    };
}
