#include "studio/file_management_dialog.hpp"

namespace studio
{
    bool FileManagementDialog::Open(AppCapabilities& capabilities, const std::string& contentPath)
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
        dialogDesc.name = "FileManagementDialog";
        dialogDesc.title = L"File";
        dialogDesc.contentPath = contentPath;
        dialogDesc.dockMode = WebDialogDockMode::Floating;
        dialogDesc.rect = { 160, 110, 520, 430 };
        dialogDesc.visible = true;
        dialogDesc.resizable = true;
        dialogDesc.alwaysOnTop = false;

        m_dialogHandle = capabilities.ui->CreateWebDialog(dialogDesc);
        return m_dialogHandle != 0U;
    }

    void FileManagementDialog::Close(AppCapabilities& capabilities)
    {
        if (m_dialogHandle == 0U || !capabilities.ui)
        {
            return;
        }

        capabilities.ui->DestroyWebDialog(m_dialogHandle);
        m_dialogHandle = 0U;
    }

    bool FileManagementDialog::IsOpen() const
    {
        return m_dialogHandle != 0U;
    }

    IAppUi::WebDialogHandle FileManagementDialog::GetHandle() const
    {
        return m_dialogHandle;
    }
}
