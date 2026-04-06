#include "filesystem.hpp"

#include <windows.h>

namespace
{
    bool g_initialized = false;

    std::filesystem::path g_executableDirectory;
    std::filesystem::path g_workingDirectory;
    std::filesystem::path g_buildDirectory;
    std::filesystem::path g_assetsDirectory;

    std::filesystem::path ResolveExecutableDirectory()
    {
        wchar_t buffer[MAX_PATH] = {};
        const DWORD length = GetModuleFileNameW(nullptr, buffer, MAX_PATH);

        if (length == 0 || length >= MAX_PATH)
        {
            return std::filesystem::current_path();
        }

        return std::filesystem::path(buffer).parent_path();
    }
}

namespace FileSystem
{
    bool Initialize()
    {
        if (g_initialized)
        {
            return true;
        }

        try
        {
            g_workingDirectory = std::filesystem::current_path();
            g_executableDirectory = ResolveExecutableDirectory();
            g_buildDirectory = g_executableDirectory / "build";
            g_assetsDirectory = g_executableDirectory / "assets";

            g_initialized = true;
            return true;
        }
        catch (...)
        {
            g_initialized = false;
            return false;
        }
    }

    const std::filesystem::path& GetExecutableDirectory()
    {
        return g_executableDirectory;
    }

    const std::filesystem::path& GetWorkingDirectory()
    {
        return g_workingDirectory;
    }

    const std::filesystem::path& GetBuildDirectory()
    {
        return g_buildDirectory;
    }

    const std::filesystem::path& GetAssetsDirectory()
    {
        return g_assetsDirectory;
    }

    std::filesystem::path GetLogFilePath()
    {
        return g_buildDirectory / "engine.log";
    }

    std::filesystem::path MakePathRelativeToExecutable(const std::filesystem::path& relativePath)
    {
        return g_executableDirectory / relativePath;
    }

    bool EnsureDirectoryExists(const std::filesystem::path& directoryPath)
    {
        try
        {
            if (std::filesystem::exists(directoryPath))
            {
                return std::filesystem::is_directory(directoryPath);
            }

            return std::filesystem::create_directories(directoryPath);
        }
        catch (...)
        {
            return false;
        }
    }

    bool Exists(const std::filesystem::path& path)
    {
        try
        {
            return std::filesystem::exists(path);
        }
        catch (...)
        {
            return false;
        }
    }

    bool IsDirectory(const std::filesystem::path& path)
    {
        try
        {
            return std::filesystem::is_directory(path);
        }
        catch (...)
        {
            return false;
        }
    }

    std::string ToString(const std::filesystem::path& path)
    {
        try
        {
            return path.string();
        }
        catch (...)
        {
            return std::string();
        }
    }
}