#include "studio/studio_build_system.hpp"

#include "studio/studio_project_system.hpp"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>

#ifdef CreateDirectory
#undef CreateDirectory
#endif

#include <utility>
#include <vector>
#include <filesystem>

namespace studio
{
    namespace
    {
        struct BuildInvocation
        {
            std::string configureCommand;
            std::string buildCommand;
        };

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

        std::string QuoteArgument(const std::string& value)
        {
            return "\"" + value + "\"";
        }

        std::wstring ToWideString(const std::string& value)
        {
            return std::wstring(value.begin(), value.end());
        }

        std::string BuildWin32ErrorMessage(const std::string& action, DWORD errorCode)
        {
            return action + " failed with Win32 error " + std::to_string(static_cast<unsigned long>(errorCode)) + ".";
        }

        // Transitional backend helper:
        // Studio currently uses a direct CMake backend while the repo converges on
        // a unified builder contract across CI, agent, Studio, and human lanes.
        // Keep raw command construction centralized here so migration to a shared
        // builder contract can replace one seam instead of many call sites.
        BuildInvocation BuildCMakeInvocation(
            const std::string& physicalProjectPath,
            const std::string& physicalBuildPath)
        {
            BuildInvocation invocation;
            invocation.configureCommand =
                "cmake -S " + QuoteArgument(physicalProjectPath) +
                " -B " + QuoteArgument(physicalBuildPath) +
                " -G Ninja";
            invocation.buildCommand = "cmake --build " + QuoteArgument(physicalBuildPath);
            return invocation;
        }
    }

    StudioBuildSystem::StudioBuildSystem()
        : StudioBuildSystem(FileProxy{})
    {
    }

    StudioBuildSystem::StudioBuildSystem(FileProxy files)
        : m_files(std::move(files))
    {
    }

    BuildAndRunProjectResult StudioBuildSystem::BuildAndRunProject(const BuildAndRunProjectRequest& request) const
    {
        BuildAndRunProjectResult result;
        result.projectId = request.projectId;

        if (request.projectId.empty() ||
            !m_files.IsValidVirtualPath(request.projectId) ||
            request.projectId.find('/') != std::string::npos ||
            request.projectId.find('\\') != std::string::npos ||
            request.projectId.find("..") != std::string::npos)
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

        const std::string projectPath = StudioProjectSystem::GetProjectSourcePath(request.projectId);
        const std::string buildPath = StudioProjectSystem::GetProjectBuildPath(request.projectId);
        result.logicalBuildPath = JoinVirtualPath(m_files.GetLogicalRoot(), buildPath);

        if (!m_files.Exists(JoinVirtualPath(projectPath, "project.enginegame")))
        {
            result.error = "Project manifest is missing: " + JoinVirtualPath(m_files.GetLogicalRoot(), JoinVirtualPath(projectPath, "project.enginegame"));
            return result;
        }

        if (!m_files.CreateDirectory(buildPath, error))
        {
            result.error = error;
            return result;
        }

        const std::string physicalProjectPath = m_files.Resolve(projectPath);
        const std::string physicalBuildPath = m_files.Resolve(buildPath);
        if (physicalProjectPath.empty() || physicalBuildPath.empty())
        {
            result.error = "Failed to resolve project or build workspace path.";
            return result;
        }

        // Transitional behavior:
        // StudioBuildSystem remains a build client, not the final build-policy owner.
        // The backend path currently invokes CMake directly through one centralized
        // helper until the unified builder contract is landed.
        const BuildInvocation invocation = BuildCMakeInvocation(physicalProjectPath, physicalBuildPath);

        if (!RunProcessAndWait(invocation.configureCommand, result.configureExitCode, error))
        {
            result.error = error;
            return result;
        }

        if (result.configureExitCode != 0)
        {
            result.error =
                "Project configure failed with exit code " +
                std::to_string(result.configureExitCode) +
                ".";
            return result;
        }

        if (!RunProcessAndWait(invocation.buildCommand, result.buildExitCode, error))
        {
            result.error = error;
            return result;
        }

        if (result.buildExitCode != 0)
        {
            result.error =
                "Project build failed with exit code " +
                std::to_string(result.buildExitCode) +
                ".";
            return result;
        }

        result.executablePath = m_files.Resolve(JoinVirtualPath(buildPath, "bin/" + request.projectId + ".exe"));
        if (result.executablePath.empty())
        {
            result.error = "Built executable path could not be resolved.";
            return result;
        }

        if (request.exportAfterBuild)
        {
            const std::string exportPath = request.projectId + "/Export";
            if (!m_files.CreateDirectory(exportPath, error))
            {
                result.error = error;
                return result;
            }

            const std::string exportedExeLogicalPath = JoinVirtualPath(exportPath, request.projectId + ".exe");
            const std::string exportedExePhysicalPath = m_files.Resolve(exportedExeLogicalPath);
            if (exportedExePhysicalPath.empty())
            {
                result.error = "Export path could not be resolved.";
                return result;
            }

            std::error_code copyError;
            std::filesystem::copy_file(
                std::filesystem::path(result.executablePath),
                std::filesystem::path(exportedExePhysicalPath),
                std::filesystem::copy_options::overwrite_existing,
                copyError);
            if (copyError)
            {
                result.error = "Failed to export executable: " + copyError.message();
                return result;
            }

            const std::string physicalExportPath = m_files.Resolve(exportPath);
            if (physicalExportPath.empty())
            {
                result.error = "Export folder could not be resolved.";
                return result;
            }

            const std::filesystem::path exportRoot(physicalExportPath);
            const std::filesystem::path projectRoot(physicalProjectPath);

            const auto copyProjectEntry =
                [&](const char* relativePath, std::string& outError) -> bool
                {
                    const std::filesystem::path source = projectRoot / relativePath;
                    if (!std::filesystem::exists(source))
                    {
                        return true;
                    }

                    const std::filesystem::path destination = exportRoot / relativePath;
                    std::error_code localError;
                    if (std::filesystem::exists(destination))
                    {
                        std::filesystem::remove_all(destination, localError);
                        if (localError)
                        {
                            outError = "Failed to replace exported " + std::string(relativePath) + ": " + localError.message();
                            return false;
                        }
                    }

                    if (std::filesystem::is_directory(source))
                    {
                        std::filesystem::create_directories(destination, localError);
                        if (localError)
                        {
                            outError = "Failed to create export folder for " + std::string(relativePath) + ": " + localError.message();
                            return false;
                        }

                        std::filesystem::copy(
                            source,
                            destination,
                            std::filesystem::copy_options::recursive | std::filesystem::copy_options::overwrite_existing,
                            localError);
                    }
                    else
                    {
                        std::filesystem::create_directories(destination.parent_path(), localError);
                        if (!localError)
                        {
                            std::filesystem::copy_file(
                                source,
                                destination,
                                std::filesystem::copy_options::overwrite_existing,
                                localError);
                        }
                    }

                    if (localError)
                    {
                        outError = "Failed to export " + std::string(relativePath) + ": " + localError.message();
                        return false;
                    }

                    return true;
                };

            if (!copyProjectEntry("project.enginegame", error) ||
                !copyProjectEntry("config", error) ||
                !copyProjectEntry("scenes", error) ||
                !copyProjectEntry("assets", error))
            {
                result.error = error;
                return result;
            }

            result.logicalExportPath = JoinVirtualPath(m_files.GetLogicalRoot(), exportPath);
            result.executablePath = exportedExePhysicalPath;
        }

        if (request.runAfterBuild)
        {
            if (!LaunchProcess(QuoteArgument(result.executablePath), error))
            {
                result.error = error;
                return result;
            }
        }

        result.success = true;
        if (request.runAfterBuild)
        {
            result.message = "Built and launched project '" + request.projectId + "'.";
        }
        else if (request.exportAfterBuild)
        {
            result.message = "Exported project '" + request.projectId + "' to " + result.logicalExportPath + ".";
        }
        else
        {
            result.message = "Built project '" + request.projectId + "'.";
        }
        return result;
    }

    bool StudioBuildSystem::RunProcessAndWait(const std::string& commandLine, int& outExitCode, std::string& outError) const
    {
        std::wstring wideCommandLine = ToWideString(commandLine);
        std::vector<wchar_t> commandBuffer(wideCommandLine.begin(), wideCommandLine.end());
        commandBuffer.push_back(L'\0');

        STARTUPINFOW startupInfo = {};
        startupInfo.cb = sizeof(startupInfo);

        PROCESS_INFORMATION processInfo = {};
        if (!CreateProcessW(
                nullptr,
                commandBuffer.data(),
                nullptr,
                nullptr,
                FALSE,
                CREATE_NO_WINDOW,
                nullptr,
                nullptr,
                &startupInfo,
                &processInfo))
        {
            outError = BuildWin32ErrorMessage("CreateProcess", GetLastError());
            return false;
        }

        WaitForSingleObject(processInfo.hProcess, INFINITE);

        DWORD exitCode = 1;
        if (!GetExitCodeProcess(processInfo.hProcess, &exitCode))
        {
            outError = BuildWin32ErrorMessage("GetExitCodeProcess", GetLastError());
            CloseHandle(processInfo.hThread);
            CloseHandle(processInfo.hProcess);
            return false;
        }

        CloseHandle(processInfo.hThread);
        CloseHandle(processInfo.hProcess);
        outExitCode = static_cast<int>(exitCode);
        return true;
    }

    bool StudioBuildSystem::LaunchProcess(const std::string& commandLine, std::string& outError) const
    {
        std::wstring wideCommandLine = ToWideString(commandLine);
        std::vector<wchar_t> commandBuffer(wideCommandLine.begin(), wideCommandLine.end());
        commandBuffer.push_back(L'\0');

        STARTUPINFOW startupInfo = {};
        startupInfo.cb = sizeof(startupInfo);

        PROCESS_INFORMATION processInfo = {};
        if (!CreateProcessW(
                nullptr,
                commandBuffer.data(),
                nullptr,
                nullptr,
                FALSE,
                0,
                nullptr,
                nullptr,
                &startupInfo,
                &processInfo))
        {
            outError = BuildWin32ErrorMessage("CreateProcess", GetLastError());
            return false;
        }

        CloseHandle(processInfo.hThread);
        CloseHandle(processInfo.hProcess);
        return true;
    }
}
