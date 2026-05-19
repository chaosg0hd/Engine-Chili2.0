#include "studio/new_project_dialog.hpp"

namespace studio
{
    bool NewProjectDialog::Open(AppCapabilities& capabilities, const std::string& contentPath)
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
        dialogDesc.name = "NewProjectDialog";
        dialogDesc.title = L"Create New Project";
        dialogDesc.contentPath = contentPath;
        dialogDesc.dockMode = WebDialogDockMode::Floating;
        dialogDesc.rect = { 180, 120, 560, 560 };
        dialogDesc.visible = true;
        dialogDesc.resizable = true;
        dialogDesc.alwaysOnTop = false;

        m_dialogHandle = capabilities.ui->CreateWebDialog(dialogDesc);
        return m_dialogHandle != 0U;
    }

    void NewProjectDialog::Draw(AppCapabilities& capabilities)
    {
        if (m_dialogHandle == 0U || !capabilities.ui)
        {
            return;
        }

        capabilities.ui->SetWebDialogVisible(m_dialogHandle, true);
    }

    void NewProjectDialog::Close(AppCapabilities& capabilities)
    {
        if (m_dialogHandle == 0U || !capabilities.ui)
        {
            return;
        }

        capabilities.ui->DestroyWebDialog(m_dialogHandle);
        m_dialogHandle = 0U;
    }

    CreateProjectResult NewProjectDialog::OnCreateClicked(const CreateProjectRequest& request)
    {
        return m_projects.CreateProject(request);
    }

    CreateProjectResult NewProjectDialog::CreateExampleProject()
    {
        CreateProjectRequest request;
        request.projectName = "Pong";
        request.templateName = "Arcade2D";
        request.overwrite = false;
        return OnCreateClicked(request);
    }

    bool NewProjectDialog::IsOpen() const
    {
        return m_dialogHandle != 0U;
    }

    IAppUi::WebDialogHandle NewProjectDialog::GetHandle() const
    {
        return m_dialogHandle;
    }

    CreateProjectResult CreateExampleProject()
    {
        StudioProjectSystem projects;

        CreateProjectRequest request;
        request.projectName = "Pong";
        request.templateName = "Arcade2D";
        request.overwrite = false;

        return projects.CreateProject(request);
    }
}
