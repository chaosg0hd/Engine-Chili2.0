#include "studio/studio_project_system.hpp"

#include <cctype>
#include <sstream>
#include <utility>

namespace studio
{
    namespace
    {
        std::string JoinVirtualPath(const std::string& left, const std::string& right)
        {
            if (left.empty())
            {
                return right;
            }

            if (right.empty())
            {
                return left;
            }

            return left + "/" + right;
        }

        std::string BuildManifest(const CreateProjectRequest& request, const std::string& projectId)
        {
            std::ostringstream manifest;
            manifest
                << "name = " << request.projectName << "\n"
                << "id = " << projectId << "\n"
                << "template = " << request.templateName << "\n"
                << "runtime = " << (projectId == "pong" ? "PongRuntime" : "HelloGameRuntime") << "\n"
                << "default_scene = scenes/main.scene\n"
                << "source_header = src/" << projectId << ".hpp\n"
                << "source_entry = src/" << projectId << ".cpp\n"
                << "target_fps = 60\n";
            return manifest.str();
        }

        bool IsSingleSegmentProjectId(const std::string& projectId)
        {
            return !projectId.empty() &&
                projectId.find('/') == std::string::npos &&
                projectId.find('\\') == std::string::npos &&
                projectId.find("..") == std::string::npos;
        }

        std::string ExtractManifestField(const std::string& manifestText, const std::string& fieldName)
        {
            const std::string prefix = fieldName + " = ";
            std::stringstream stream(manifestText);
            std::string line;

            while (std::getline(stream, line))
            {
                if (line.find(prefix) == 0U)
                {
                    return line.substr(prefix.size());
                }
            }

            return std::string();
        }
    }

    StudioProjectSystem::StudioProjectSystem()
        : StudioProjectSystem(FileProxy{})
    {
    }

    StudioProjectSystem::StudioProjectSystem(FileProxy files)
        : m_files(std::move(files))
        , m_templates(m_files)
    {
    }

    CreateProjectResult StudioProjectSystem::CreateProject(const CreateProjectRequest& request)
    {
        CreateProjectResult result;
        result.projectId = MakeProjectId(request.projectName);

        if (request.projectName.empty())
        {
            result.error = "Project name is required.";
            return result;
        }

        if (result.projectId.empty())
        {
            result.error = "Project name does not contain any safe project id characters.";
            return result;
        }

        if (request.templateName.empty())
        {
            result.error = "Template name is required.";
            return result;
        }

        std::string error;
        if (!m_files.EnsureWorkspaceFolders(error))
        {
            result.error = error;
            return result;
        }

        if (!IsSingleSegmentProjectId(result.projectId))
        {
            result.error = "Project id must be a single safe path segment.";
            return result;
        }

        const std::string projectPath = GetProjectSourcePath(result.projectId);
        result.logicalProjectPath = JoinVirtualPath(m_files.GetLogicalRoot(), projectPath);

        if (m_files.Exists(GetProjectWorkspacePath(result.projectId)) && !request.overwrite)
        {
            result.error = "Project already exists: " + JoinVirtualPath(m_files.GetLogicalRoot(), GetProjectWorkspacePath(result.projectId));
            return result;
        }

        if (!CreateProjectFolders(result.projectId, error))
        {
            result.error = error;
            return result;
        }

        if (!WriteManifest(request, result.projectId, projectPath, error))
        {
            result.error = error;
            return result;
        }

        const ApplyTemplateResult templateResult = m_templates.ApplyTemplate(
            request.templateName,
            projectPath,
            request.projectName,
            result.projectId);
        if (!templateResult.success)
        {
            result.error = templateResult.error;
            return result;
        }

        result.success = true;
        result.message = "Created project '" + request.projectName + "' at " + result.logicalProjectPath + ".";
        m_currentProject.isOpen = true;
        m_currentProject.projectId = result.projectId;
        m_currentProject.projectName = request.projectName;
        m_currentProject.logicalProjectPath = result.logicalProjectPath;
        m_currentProject.projectRootPath = projectPath;
        m_currentProject.runtimeName = result.projectId == "pong" ? "PongRuntime" : "HelloGameRuntime";
        m_currentProject.defaultScenePath = "scenes/main.scene";
        m_currentProject.sourceEntryPath = "src/" + result.projectId + ".cpp";
        m_currentProject.manifestText = BuildManifest(request, result.projectId);
        return result;
    }

    OpenProjectResult StudioProjectSystem::OpenProject(const OpenProjectRequest& request)
    {
        OpenProjectResult result;
        result.projectId = MakeProjectId(request.projectId);

        if (result.projectId.empty() || !IsSingleSegmentProjectId(result.projectId))
        {
            result.error = "A valid project id is required.";
            return result;
        }

        std::string error;
        if (!m_files.EnsureWorkspaceFolders(error))
        {
            result.error = error;
            return result;
        }

        const std::string projectPath = GetProjectSourcePath(result.projectId);
        const std::string manifestPath = JoinVirtualPath(projectPath, "project.enginegame");
        result.logicalProjectPath = JoinVirtualPath(m_files.GetLogicalRoot(), projectPath);

        if (!m_files.Exists(manifestPath))
        {
            result.error = "Project manifest was not found: " + JoinVirtualPath(m_files.GetLogicalRoot(), manifestPath);
            return result;
        }

        if (!m_files.ReadText(manifestPath, result.manifestText, error))
        {
            result.error = error;
            return result;
        }

        result.projectName = ExtractManifestField(result.manifestText, "name");
        if (result.projectName.empty())
        {
            result.projectName = result.projectId;
        }
        result.runtimeName = ExtractManifestField(result.manifestText, "runtime");
        if (result.runtimeName.empty())
        {
            result.runtimeName = result.projectId == "pong" ? "PongRuntime" : "HelloGameRuntime";
        }
        result.defaultScenePath = ExtractManifestField(result.manifestText, "default_scene");
        result.sourceEntryPath = ExtractManifestField(result.manifestText, "source_entry");

        result.success = true;
        result.message = "Opened project '" + result.projectName + "' at " + result.logicalProjectPath + ".";
        m_currentProject.isOpen = true;
        m_currentProject.projectId = result.projectId;
        m_currentProject.projectName = result.projectName;
        m_currentProject.logicalProjectPath = result.logicalProjectPath;
        m_currentProject.projectRootPath = projectPath;
        m_currentProject.runtimeName = result.runtimeName;
        m_currentProject.defaultScenePath = result.defaultScenePath;
        m_currentProject.sourceEntryPath = result.sourceEntryPath;
        m_currentProject.manifestText = result.manifestText;
        return result;
    }

    SaveProjectResult StudioProjectSystem::SaveProject(const SaveProjectRequest& request)
    {
        SaveProjectResult result;
        result.projectId = MakeProjectId(request.projectId);

        if (result.projectId.empty() || !IsSingleSegmentProjectId(result.projectId))
        {
            result.error = "A valid project id is required.";
            return result;
        }

        std::string error;
        if (!m_files.EnsureWorkspaceFolders(error))
        {
            result.error = error;
            return result;
        }

        const std::string projectPath = GetProjectSourcePath(result.projectId);
        const std::string manifestPath = JoinVirtualPath(projectPath, "project.enginegame");
        result.logicalProjectPath = JoinVirtualPath(m_files.GetLogicalRoot(), projectPath);

        if (!m_files.Exists(manifestPath))
        {
            result.error = "Cannot save because project manifest was not found: " + JoinVirtualPath(m_files.GetLogicalRoot(), manifestPath);
            return result;
        }

        const std::string savePath = JoinVirtualPath(GetProjectWorkspacePath(result.projectId), "Project/studio.session");
        if (!m_files.WriteText(savePath, request.editorState, error))
        {
            result.error = error;
            return result;
        }

        result.logicalSavePath = JoinVirtualPath(m_files.GetLogicalRoot(), savePath);
        result.success = true;
        result.message = "Saved project session for '" + result.projectId + "' to " + result.logicalSavePath + ".";
        return result;
    }

    StudioProject StudioProjectSystem::GetCurrentProject() const
    {
        return m_currentProject;
    }

    std::string StudioProjectSystem::GetCurrentProjectRoot() const
    {
        return m_currentProject.isOpen ? GetProjectSourcePath(m_currentProject.projectId) : std::string();
    }

    std::string StudioProjectSystem::MakeProjectId(const std::string& projectName)
    {
        std::string projectId;
        bool needsSeparator = false;

        for (const char character : projectName)
        {
            const unsigned char value = static_cast<unsigned char>(character);
            if (std::isalnum(value) != 0)
            {
                if (needsSeparator && !projectId.empty())
                {
                    projectId += '_';
                }

                projectId += static_cast<char>(std::tolower(value));
                needsSeparator = false;
            }
            else
            {
                needsSeparator = !projectId.empty();
            }
        }

        return projectId;
    }

    std::string StudioProjectSystem::GetProjectWorkspacePath(const std::string& projectId)
    {
        return projectId;
    }

    std::string StudioProjectSystem::GetProjectSourcePath(const std::string& projectId)
    {
        return JoinVirtualPath(GetProjectWorkspacePath(projectId), "Project");
    }

    std::string StudioProjectSystem::GetProjectBuildPath(const std::string& projectId)
    {
        return JoinVirtualPath(GetProjectWorkspacePath(projectId), "Build");
    }

    std::string StudioProjectSystem::GetProjectCachePath(const std::string& projectId)
    {
        return JoinVirtualPath(GetProjectWorkspacePath(projectId), "Cache");
    }

    std::string StudioProjectSystem::GetProjectLogsPath(const std::string& projectId)
    {
        return JoinVirtualPath(GetProjectWorkspacePath(projectId), "Logs");
    }

    bool StudioProjectSystem::CreateProjectFolders(const std::string& projectId, std::string& outError) const
    {
        const std::string projectPath = GetProjectSourcePath(projectId);
        return m_files.CreateDirectory(GetProjectWorkspacePath(projectId), outError) &&
            m_files.CreateDirectory(projectPath, outError) &&
            m_files.CreateDirectory(GetProjectBuildPath(projectId), outError) &&
            m_files.CreateDirectory(GetProjectCachePath(projectId), outError) &&
            m_files.CreateDirectory(GetProjectLogsPath(projectId), outError) &&
            m_files.CreateDirectory(JoinVirtualPath(projectPath, "src"), outError) &&
            m_files.CreateDirectory(JoinVirtualPath(projectPath, "assets"), outError) &&
            m_files.CreateDirectory(JoinVirtualPath(projectPath, "scenes"), outError) &&
            m_files.CreateDirectory(JoinVirtualPath(projectPath, "config"), outError);
    }

    bool StudioProjectSystem::WriteManifest(
        const CreateProjectRequest& request,
        const std::string& projectId,
        const std::string& projectPath,
        std::string& outError) const
    {
        return m_files.WriteText(
            JoinVirtualPath(projectPath, "project.enginegame"),
            BuildManifest(request, projectId),
            outError);
    }

    CreateProjectResult CreateExampleProjectForSmokeTest()
    {
        StudioProjectSystem projects;

        CreateProjectRequest request;
        request.projectName = "Pong";
        request.templateName = "Arcade2D";
        request.overwrite = true;

        return projects.CreateProject(request);
    }
}
