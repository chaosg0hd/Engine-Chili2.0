#include "studio/studio_console_panel.hpp"

namespace studio
{
    bool StudioConsolePanel::Open(AppCapabilities& capabilities, const std::string& contentPath, int dockInsetLeft, int dockInsetRight, int dockHeight)
    {
        if (!capabilities.ui)
        {
            return false;
        }

        if (m_dialogHandle != 0U)
        {
            return SetVisible(capabilities, true);
        }

        WebDialogDesc dialogDesc;
        dialogDesc.name = "StudioConsole";
        dialogDesc.title = L"Studio Console";
        dialogDesc.contentPath = contentPath;
        dialogDesc.dockMode = WebDialogDockMode::Bottom;
        dialogDesc.dockSize = dockHeight;
        dialogDesc.dockInsetLeft = dockInsetLeft;
        dialogDesc.dockInsetRight = dockInsetRight;
        dialogDesc.visible = true;
        dialogDesc.resizable = false;

        m_dialogHandle = capabilities.ui->CreateWebDialog(dialogDesc);
        m_visible = m_dialogHandle != 0U;
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
        m_visible = false;
    }

    bool StudioConsolePanel::SetVisible(AppCapabilities& capabilities, bool visible)
    {
        if (m_dialogHandle == 0U || !capabilities.ui)
        {
            return false;
        }

        const bool updated = capabilities.ui->SetWebDialogVisible(m_dialogHandle, visible);
        if (updated)
        {
            m_visible = visible;
        }

        return updated;
    }

    bool StudioConsolePanel::IsVisible() const
    {
        return m_dialogHandle != 0U && m_visible;
    }
}
