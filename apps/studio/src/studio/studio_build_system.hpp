#pragma once

#include "studio/file_proxy.hpp"

#include <string>

namespace studio
{
    struct BuildAndRunProjectRequest
    {
        std::string projectId;
        bool runAfterBuild = true;
        bool exportAfterBuild = false;
    };

    struct BuildAndRunProjectResult
    {
        bool success = false;
        std::string projectId;
        std::string logicalBuildPath;
        std::string runtimeOutputPath;
        std::string executablePath;
        std::string logicalExportPath;
        std::string message;
        std::string error;
        int configureExitCode = -1;
        int buildExitCode = -1;
    };

    class StudioBuildSystem
    {
    public:
        StudioBuildSystem();
        explicit StudioBuildSystem(FileProxy files);

        BuildAndRunProjectResult BuildAndRunProject(const BuildAndRunProjectRequest& request) const;

    private:
        bool RunProcessAndWait(
            const std::string& commandLine,
            const std::string& workingDirectory,
            int& outExitCode,
            std::string& outError) const;
        bool LaunchProcess(
            const std::string& commandLine,
            const std::string& workingDirectory,
            std::string& outError) const;

        FileProxy m_files;
    };
}
