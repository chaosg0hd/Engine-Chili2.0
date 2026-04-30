#pragma once

#include "studio/file_proxy.hpp"
#include "studio/studio_project_system.hpp"

#include <string>

namespace studio
{
    struct RenameFileActionRequest
    {
        std::string logicalPath;
        std::string newName;
    };

    struct RenameFileActionResult
    {
        bool success = false;
        std::string oldLogicalPath;
        std::string newLogicalPath;
        std::string message;
        std::string error;
    };

    class StudioFileActions
    {
    public:
        StudioFileActions();
        explicit StudioFileActions(FileProxy files);

        RenameFileActionResult Rename(const StudioProjectSystem& projects, const RenameFileActionRequest& request) const;

    private:
        bool IsInsideProjectRoot(const std::string& projectRoot, const std::string& logicalPath) const;
        bool IsValidNewName(const std::string& newName, std::string& outError) const;
        std::string GetParentPath(const std::string& logicalPath) const;

        FileProxy m_files;
    };
}
