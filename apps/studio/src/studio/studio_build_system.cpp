#include "studio/studio_build_system.hpp"

#include "runtime/runtime_game_api.hpp"
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

#include <algorithm>
#include <filesystem>
#include <sstream>
#include <utility>
#include <vector>

namespace studio
{
    namespace
    {
        struct BuildInvocation
        {
            std::string configureCommand;
            std::string buildCommand;
        };

        struct ProjectBuildContract
        {
            studio_runtime::ProjectCodeEntryKind codeEntryKind = studio_runtime::ProjectCodeEntryKind::NativeInProcess;
            std::string configureCommand;
            std::string buildCommand;
            std::string runtimeOutput;
            std::string scriptEntry;
            std::string adapterEntry;
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

        studio_runtime::ProjectCodeEntryKind ParseProjectCodeEntryKind(const std::string& value)
        {
            if (value == "native_artifact")
            {
                return studio_runtime::ProjectCodeEntryKind::NativeArtifact;
            }
            if (value == "lua")
            {
                return studio_runtime::ProjectCodeEntryKind::LuaScript;
            }
            if (value == "external_adapter")
            {
                return studio_runtime::ProjectCodeEntryKind::ExternalAdapter;
            }

            return studio_runtime::ProjectCodeEntryKind::NativeInProcess;
        }

        std::string NormalizeSlashes(std::string value)
        {
            std::replace(value.begin(), value.end(), '\\', '/');
            return value;
        }

        std::string ResolveProjectPath(
            const studio::FileProxy& files,
            const std::string& projectPath,
            const std::string& relativeOrAbsolutePath)
        {
            if (relativeOrAbsolutePath.empty())
            {
                return std::string();
            }

            std::filesystem::path path(relativeOrAbsolutePath);
            if (path.is_absolute())
            {
                return path.string();
            }

            const std::string physicalProject = files.Resolve(projectPath);
            if (physicalProject.empty())
            {
                return std::string();
            }

            std::error_code ec;
            const std::filesystem::path resolved = std::filesystem::weakly_canonical(
                std::filesystem::path(physicalProject) / NormalizeSlashes(relativeOrAbsolutePath), ec);
            return ec ? std::string() : resolved.string();
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

        ProjectBuildContract BuildProjectContract(
            const std::string& manifestText,
            const std::string& projectId,
            const std::string& physicalProjectPath,
            const std::string& physicalBuildPath)
        {
            ProjectBuildContract contract;
            contract.codeEntryKind = ParseProjectCodeEntryKind(ExtractManifestField(manifestText, "runtime_kind"));
            contract.configureCommand = ExtractManifestField(manifestText, "build_configure");
            contract.buildCommand = ExtractManifestField(manifestText, "build_command");
            contract.runtimeOutput = ExtractManifestField(manifestText, "build_output");
            contract.scriptEntry = ExtractManifestField(manifestText, "script_entry");
            contract.adapterEntry = ExtractManifestField(manifestText, "adapter_entry");

            if (contract.configureCommand.empty() &&
                contract.buildCommand.empty() &&
                contract.codeEntryKind != studio_runtime::ProjectCodeEntryKind::LuaScript)
            {
                const BuildInvocation invocation = BuildCMakeInvocation(physicalProjectPath, physicalBuildPath);
                contract.configureCommand = invocation.configureCommand;
                contract.buildCommand = invocation.buildCommand;
            }

            if (contract.runtimeOutput.empty())
            {
                switch (contract.codeEntryKind)
                {
                case studio_runtime::ProjectCodeEntryKind::NativeArtifact:
                    contract.runtimeOutput = ExtractManifestField(manifestText, "runtime_artifact");
                    if (contract.runtimeOutput.empty())
                    {
                        contract.runtimeOutput = "../Build/bin/" + projectId + ".dll";
                    }
                    break;
                case studio_runtime::ProjectCodeEntryKind::ExternalAdapter:
                    contract.runtimeOutput = contract.adapterEntry.empty()
                        ? ("../Build/bin/" + projectId + ".exe")
                        : contract.adapterEntry;
                    break;
                case studio_runtime::ProjectCodeEntryKind::LuaScript:
                    contract.runtimeOutput = contract.scriptEntry;
                    break;
                case studio_runtime::ProjectCodeEntryKind::NativeInProcess:
                default:
                    contract.runtimeOutput = "../Build/bin/" + projectId + ".exe";
                    break;
                }
            }

            return contract;
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
        const std::string manifestPath = JoinVirtualPath(projectPath, "project.enginegame");

        if (!m_files.Exists(manifestPath))
        {
            result.error = "Project manifest is missing: " + JoinVirtualPath(m_files.GetLogicalRoot(), manifestPath);
            return result;
        }

        std::string manifestText;
        if (!m_files.ReadText(manifestPath, manifestText, error))
        {
            result.error = error;
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

        const ProjectBuildContract contract = BuildProjectContract(
            manifestText,
            request.projectId,
            physicalProjectPath,
            physicalBuildPath);

        if (!contract.configureCommand.empty())
        {
            if (!RunProcessAndWait(contract.configureCommand, physicalProjectPath, result.configureExitCode, error))
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
        }

        if (!contract.buildCommand.empty())
        {
            if (!RunProcessAndWait(contract.buildCommand, physicalProjectPath, result.buildExitCode, error))
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
        }

        result.runtimeOutputPath = ResolveProjectPath(m_files, projectPath, contract.runtimeOutput);
        result.executablePath = result.runtimeOutputPath;

        const bool requiresRuntimeOutput =
            contract.codeEntryKind != studio_runtime::ProjectCodeEntryKind::LuaScript;
        if (requiresRuntimeOutput &&
            (result.runtimeOutputPath.empty() || !std::filesystem::exists(std::filesystem::path(result.runtimeOutputPath))))
        {
            result.error = "Configured runtime output was not produced: " + contract.runtimeOutput;
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
                !copyProjectEntry("project.chili.json", error) ||
                !copyProjectEntry("config", error) ||
                !copyProjectEntry("scenes", error) ||
                !copyProjectEntry("assets", error) ||
                !copyProjectEntry("scripts", error))
            {
                result.error = error;
                return result;
            }

            if (contract.codeEntryKind == studio_runtime::ProjectCodeEntryKind::NativeInProcess ||
                contract.codeEntryKind == studio_runtime::ProjectCodeEntryKind::ExternalAdapter)
            {
                const std::string exportedExeLogicalPath = JoinVirtualPath(exportPath, request.projectId + ".exe");
                const std::string exportedExePhysicalPath = m_files.Resolve(exportedExeLogicalPath);
                if (exportedExePhysicalPath.empty())
                {
                    result.error = "Export path could not be resolved.";
                    return result;
                }

                std::error_code copyError;
                std::filesystem::copy_file(
                    std::filesystem::path(result.runtimeOutputPath),
                    std::filesystem::path(exportedExePhysicalPath),
                    std::filesystem::copy_options::overwrite_existing,
                    copyError);
                if (copyError)
                {
                    result.error = "Failed to export executable: " + copyError.message();
                    return result;
                }

                result.executablePath = exportedExePhysicalPath;
            }
            else if (contract.codeEntryKind == studio_runtime::ProjectCodeEntryKind::NativeArtifact)
            {
                const std::filesystem::path sourceArtifact(result.runtimeOutputPath);
                const std::filesystem::path artifactDestination = exportRoot / sourceArtifact.filename();
                std::error_code copyError;
                std::filesystem::copy_file(
                    sourceArtifact,
                    artifactDestination,
                    std::filesystem::copy_options::overwrite_existing,
                    copyError);
                if (copyError)
                {
                    result.error = "Failed to export runtime artifact: " + copyError.message();
                    return result;
                }

                result.runtimeOutputPath = artifactDestination.string();
                result.executablePath.clear();
            }
            else
            {
                result.executablePath.clear();
            }

            result.logicalExportPath = JoinVirtualPath(m_files.GetLogicalRoot(), exportPath);
        }

        if (request.runAfterBuild)
        {
            if (contract.codeEntryKind == studio_runtime::ProjectCodeEntryKind::LuaScript)
            {
                result.error = "Lua script projects do not have a direct launchable runtime output yet.";
                return result;
            }

            if (contract.codeEntryKind == studio_runtime::ProjectCodeEntryKind::NativeArtifact)
            {
                result.error = "Native artifact projects do not have a launch host wired up yet.";
                return result;
            }

            if (result.executablePath.empty())
            {
                result.error = "No launchable executable output was configured.";
                return result;
            }

            if (!LaunchProcess(QuoteArgument(result.executablePath), physicalProjectPath, error))
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

    bool StudioBuildSystem::RunProcessAndWait(
        const std::string& commandLine,
        const std::string& workingDirectory,
        int& outExitCode,
        std::string& outError) const
    {
        std::wstring wideCommandLine = ToWideString(commandLine);
        std::vector<wchar_t> commandBuffer(wideCommandLine.begin(), wideCommandLine.end());
        commandBuffer.push_back(L'\0');
        std::wstring wideWorkingDirectory = ToWideString(workingDirectory);

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
                wideWorkingDirectory.empty() ? nullptr : wideWorkingDirectory.c_str(),
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

    bool StudioBuildSystem::LaunchProcess(
        const std::string& commandLine,
        const std::string& workingDirectory,
        std::string& outError) const
    {
        std::wstring wideCommandLine = ToWideString(commandLine);
        std::vector<wchar_t> commandBuffer(wideCommandLine.begin(), wideCommandLine.end());
        commandBuffer.push_back(L'\0');
        std::wstring wideWorkingDirectory = ToWideString(workingDirectory);

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
                wideWorkingDirectory.empty() ? nullptr : wideWorkingDirectory.c_str(),
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
