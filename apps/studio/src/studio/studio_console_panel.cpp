#include "studio/studio_console_panel.hpp"

namespace studio
{
    bool StudioConsolePanel::Open(AppCapabilities& capabilities, const std::string& contentPath, int dockInsetLeft, int dockInsetRight)
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
        dialogDesc.name = "StudioConsole";
        dialogDesc.title = L"Studio Console";
        dialogDesc.contentPath = contentPath;
        dialogDesc.dockMode = WebDialogDockMode::Bottom;
        dialogDesc.dockSize = 176;
        dialogDesc.dockInsetLeft = dockInsetLeft;
        dialogDesc.dockInsetRight = dockInsetRight;
        dialogDesc.visible = true;
        dialogDesc.resizable = false;

        m_dialogHandle = capabilities.ui->CreateWebDialog(dialogDesc);
        return m_dialogHandle != 0U;
    }

    void StudioConsolePanel::Close(AppCapabilities& capabilities)
    {
        if (m_dialogHandle == 0U || !capabilities.ui)
        {
            return;
        }

        capabilities.ui->DestroyWebDialog(m_dialogHandle);
        m_dialogHandle = 0U;
    }
}
