#pragma once

#include "studio/file_proxy.hpp"
#include "studio/studio_template_system.hpp"

#include <string>

namespace studio
{
    struct CreateProjectRequest
    {
        std::string projectName;
        std::string templateName = "Arcade2D";
        bool overwrite = false;
    };

    struct CreateProjectResult
    {
        bool success = false;
        std::string projectId;
        std::string logicalProjectPath;
        std::string message;
        std::string error;
    };

    struct OpenProjectRequest
    {
        std::string projectId;
    };

    struct OpenProjectResult
    {
        bool success = false;
        std::string projectId;
        std::string logicalProjectPath;
        std::string projectName;
        std::string previewRuntimeName;
        std::string defaultScenePath;
        std::string sourceEntryPath;
        std::string assetProxyFolder;
        std::string manifestText;
        std::string message;
        std::string error;
    };

    struct SaveProjectRequest
    {
        std::string projectId;
        std::string editorState = "workspace=open\n";
    };

    struct SaveProjectResult
    {
        bool success = false;
        std::string projectId;
        std::string logicalProjectPath;
        std::string logicalSavePath;
        std::string message;
        std::string error;
    };

    struct StudioProject
    {
        bool isOpen = false;
        std::string projectId;
        std::string projectName;
        std::string logicalProjectPath;
        std::string projectRootPath;
        // TEMPORARY: in-process preview runtime name. See ProjectRuntimeDesc::previewRuntimeName.
        std::string previewRuntimeName;
        std::string defaultScenePath;
        std::string sourceEntryPath;
        std::string assetProxyFolder;
        std::string manifestText;
    };

    class StudioProjectSystem
    {
    public:
        StudioProjectSystem();
        explicit StudioProjectSystem(FileProxy files);

        CreateProjectResult CreateProject(const CreateProjectRequest& request);
        OpenProjectResult OpenProject(const OpenProjectRequest& request);
        SaveProjectResult SaveProject(const SaveProjectRequest& request);
        StudioProject GetCurrentProject() const;
        std::string GetCurrentProjectRoot() const;

        static std::string MakeProjectId(const std::string& projectName);
        static std::string GetProjectWorkspacePath(const std::string& projectId);
        static std::string GetProjectSourcePath(const std::string& projectId);
        static std::string GetProjectBuildPath(const std::string& projectId);
        static std::string GetProjectCachePath(const std::string& projectId);
        static std::string GetProjectLogsPath(const std::string& projectId);

    private:
        bool CreateProjectFolders(const std::string& projectId, std::string& outError) const;
        bool WriteManifest(const CreateProjectRequest& request, const std::string& projectId, const std::string& projectPath, std::string& outError) const;

        FileProxy m_files;
        StudioTemplateSystem m_templates;
        StudioProject m_currentProject;
    };

    CreateProjectResult CreateExampleProjectForSmokeTest();
}
