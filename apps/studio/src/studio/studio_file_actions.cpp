#include "studio/studio_file_actions.hpp"

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

        bool ContainsInvalidFilenameCharacter(const std::string& value)
        {
            for (const char character : value)
            {
                const unsigned char byte = static_cast<unsigned char>(character);
                if (byte < 32U)
                {
                    return true;
                }

                switch (character)
                {
                case '<':
                case '>':
                case ':':
                case '"':
                case '/':
                case '\\':
                case '|':
                case '?':
                case '*':
                    return true;
                default:
                    break;
                }
            }

            return false;
        }
    }

    StudioFileActions::StudioFileActions()
        : StudioFileActions(FileProxy{})
    {
    }

    StudioFileActions::StudioFileActions(FileProxy files)
        : m_files(std::move(files))
    {
    }

    RenameFileActionResult StudioFileActions::Rename(const StudioProjectSystem& projects, const RenameFileActionRequest& request) const
    {
        RenameFileActionResult result;
        result.oldLogicalPath = m_files.NormalizeVirtualPath(request.logicalPath);

        const StudioProject currentProject = projects.GetCurrentProject();
        const std::string projectRoot = projects.GetCurrentProjectRoot();
        if (!currentProject.isOpen || projectRoot.empty())
        {
            result.error = "No project open.";
            return result;
        }

        if (result.oldLogicalPath.empty())
        {
            result.error = "A path is required to rename.";
            return result;
        }

        if (result.oldLogicalPath == projectRoot)
        {
            result.error = "Cannot rename the current project root.";
            return result;
        }

        if (!IsInsideProjectRoot(projectRoot, result.oldLogicalPath))
        {
            result.error = "Cannot rename paths outside the current project.";
            return result;
        }

        if (!m_files.Exists(result.oldLogicalPath))
        {
            result.error = "Path does not exist: " + result.oldLogicalPath;
            return result;
        }

        std::string validationError;
        if (!IsValidNewName(request.newName, validationError))
        {
            result.error = validationError;
            return result;
        }

        const std::string parentPath = GetParentPath(result.oldLogicalPath);
        if (parentPath.empty() || !IsInsideProjectRoot(projectRoot, parentPath))
        {
            result.error = "Cannot resolve the parent folder for this item.";
            return result;
        }

        result.newLogicalPath = JoinVirtualPath(parentPath, request.newName);
        if (result.newLogicalPath == result.oldLogicalPath)
        {
            result.error = "The new name is the same as the current name.";
            return result;
        }

        if (!IsInsideProjectRoot(projectRoot, result.newLogicalPath))
        {
            result.error = "Rename target would leave the current project.";
            return result;
        }

        if (m_files.Exists(result.newLogicalPath))
        {
            result.error = "An item with that name already exists.";
            return result;
        }

        std::string error;
        if (!m_files.Rename(result.oldLogicalPath, result.newLogicalPath, error))
        {
            result.error = error;
            return result;
        }

        result.success = true;
        result.message = "Renamed to " + request.newName + ".";
        return result;
    }

    bool StudioFileActions::IsInsideProjectRoot(const std::string& projectRoot, const std::string& logicalPath) const
    {
        return logicalPath == projectRoot ||
            (logicalPath.size() > projectRoot.size() &&
                logicalPath.find(projectRoot + "/") == 0U);
    }

    bool StudioFileActions::IsValidNewName(const std::string& newName, std::string& outError) const
    {
        if (newName.empty())
        {
            outError = "New name cannot be empty.";
            return false;
        }

        if (newName == "." || newName == ".." || newName.find("..") != std::string::npos)
        {
            outError = "New name cannot contain path traversal.";
            return false;
        }

        if (ContainsInvalidFilenameCharacter(newName))
        {
            outError = "New name contains characters that are not allowed in file names.";
            return false;
        }

        if (!newName.empty() && (newName.front() == ' ' || newName.back() == ' ' || newName.back() == '.'))
        {
            outError = "New name cannot start with a space or end with a space or dot.";
            return false;
        }

        return true;
    }

    std::string StudioFileActions::GetParentPath(const std::string& logicalPath) const
    {
        const std::size_t separator = logicalPath.find_last_of('/');
        if (separator == std::string::npos)
        {
            return std::string();
        }

        return logicalPath.substr(0, separator);
    }
}
