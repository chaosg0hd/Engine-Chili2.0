#include "studio/project_explorer_panel.hpp"

#include <algorithm>
#include <sstream>

namespace studio
{
    namespace
    {
        std::string EscapeJson(const std::string& value)
        {
            std::string escaped;
            escaped.reserve(value.size());

            for (const char character : value)
            {
                if (character == '\\' || character == '"')
                {
                    escaped.push_back('\\');
                }

                escaped.push_back(character);
            }

            return escaped;
        }

        bool SortFileEntries(const FileEntry& left, const FileEntry& right)
        {
            if (left.isDirectory != right.isDirectory)
            {
                return left.isDirectory && !right.isDirectory;
            }

            return left.name < right.name;
        }
    }

    bool ProjectExplorerPanel::Open(AppCapabilities& capabilities, const std::string& contentPath, int dockInsetTop, int dockWidth)
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
        dialogDesc.name = "ProjectExplorer";
        dialogDesc.title = L"Project Explorer";
        dialogDesc.contentPath = contentPath;
        dialogDesc.dockMode = WebDialogDockMode::Right;
        dialogDesc.dockSize = dockWidth;
        dialogDesc.dockInsetTop = dockInsetTop;
        dialogDesc.visible = true;
        dialogDesc.resizable = false;

        m_dialogHandle = capabilities.ui->CreateWebDialog(dialogDesc);
        return m_dialogHandle != 0U;
    }

    void ProjectExplorerPanel::Close(AppCapabilities& capabilities)
    {
        if (m_dialogHandle == 0U || !capabilities.ui)
        {
            return;
        }

        capabilities.ui->DestroyWebDialog(m_dialogHandle);
        m_dialogHandle = 0U;
    }

    std::string ProjectExplorerPanel::BuildTreeJson(const StudioProjectSystem& projects) const
    {
        const StudioProject currentProject = projects.GetCurrentProject();
        if (!currentProject.isOpen)
        {
            return "{\"ok\":true,\"hasProject\":false,\"message\":\"No project open.\"}";
        }

        std::string error;
        const std::string rootJson = BuildEntryJson(
            projects.GetCurrentProjectRoot(),
            currentProject.projectName.empty() ? currentProject.projectId : currentProject.projectName,
            true,
            error);
        if (!error.empty())
        {
            return std::string("{\"ok\":false,\"hasProject\":true,\"message\":\"") + EscapeJson(error) + "\"}";
        }

        return std::string("{\"ok\":true,\"hasProject\":true,\"projectName\":\"") +
            EscapeJson(currentProject.projectName) +
            "\",\"projectId\":\"" +
            EscapeJson(currentProject.projectId) +
            "\",\"selectedPath\":\"" +
            EscapeJson(m_selectedFileLogicalPath) +
            "\",\"root\":" +
            rootJson +
            "}";
    }

    bool ProjectExplorerPanel::SelectFile(const StudioProjectSystem& projects, const std::string& logicalPath, std::string& outMessage)
    {
        const std::string projectRoot = projects.GetCurrentProjectRoot();
        const std::string normalizedPath = m_files.NormalizeVirtualPath(logicalPath);
        if (projectRoot.empty())
        {
            outMessage = "No project open.";
            return false;
        }

        if (!IsInsideProjectRoot(projectRoot, normalizedPath))
        {
            outMessage = "Selected path is outside the current project.";
            return false;
        }

        if (!m_files.Exists(normalizedPath))
        {
            outMessage = "Selected file does not exist.";
            return false;
        }

        if (m_files.IsDirectory(normalizedPath))
        {
            outMessage = "Selected path is a directory.";
            return false;
        }

        m_selectedFileLogicalPath = normalizedPath;
        outMessage = "Selected " + m_selectedFileLogicalPath + ".";
        return true;
    }

    const std::string& ProjectExplorerPanel::GetSelectedFileLogicalPath() const
    {
        return m_selectedFileLogicalPath;
    }

    void ProjectExplorerPanel::SetSelectedLogicalPath(const std::string& logicalPath)
    {
        m_selectedFileLogicalPath = m_files.NormalizeVirtualPath(logicalPath);
    }

    std::string ProjectExplorerPanel::BuildEntryJson(
        const std::string& logicalPath,
        const std::string& displayName,
        bool isRoot,
        std::string& outError) const
    {
        const bool isDirectory = m_files.IsDirectory(logicalPath);
        std::ostringstream output;
        output
            << "{\"name\":\"" << EscapeJson(displayName)
            << "\",\"logicalPath\":\"" << EscapeJson(logicalPath)
            << "\",\"isDirectory\":" << (isDirectory ? "true" : "false");

        if (isDirectory)
        {
            std::vector<FileEntry> entries;
            if (!m_files.ListDirectory(logicalPath, entries, outError))
            {
                return std::string();
            }

            std::sort(entries.begin(), entries.end(), SortFileEntries);
            output << ",\"children\":[";
            for (std::size_t index = 0; index < entries.size(); ++index)
            {
                if (index > 0U)
                {
                    output << ",";
                }

                output << BuildEntryJson(entries[index].logicalPath, entries[index].name, false, outError);
                if (!outError.empty())
                {
                    return std::string();
                }
            }

            output << "]";
        }

        output << ",\"isRoot\":" << (isRoot ? "true" : "false") << "}";
        return output.str();
    }

    bool ProjectExplorerPanel::IsInsideProjectRoot(const std::string& projectRoot, const std::string& logicalPath) const
    {
        return logicalPath == projectRoot ||
            (logicalPath.size() > projectRoot.size() &&
                logicalPath.find(projectRoot + "/") == 0U);
    }
}
