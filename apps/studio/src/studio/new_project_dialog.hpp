#pragma once

#include "app/app_capabilities.hpp"
#include "studio/studio_project_system.hpp"

#include <string>

namespace studio
{
    class NewProjectDialog
    {
    public:
        bool Open(AppCapabilities& capabilities, const std::string& contentPath);
        void Draw(AppCapabilities& capabilities);
        void Close(AppCapabilities& capabilities);

        CreateProjectResult OnCreateClicked(const CreateProjectRequest& request);
        CreateProjectResult CreateExampleProject();

        bool IsOpen() const;
        IAppUi::WebDialogHandle GetHandle() const;

    private:
        StudioProjectSystem m_projects;
        IAppUi::WebDialogHandle m_dialogHandle = 0U;
    };

    CreateProjectResult CreateExampleProject();
}
