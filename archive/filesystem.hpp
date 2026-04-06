#pragma once

#include <filesystem>
#include <string>

namespace FileSystem
{
    bool Initialize();

    const std::filesystem::path& GetExecutableDirectory();
    const std::filesystem::path& GetWorkingDirectory();
    const std::filesystem::path& GetBuildDirectory();
    const std::filesystem::path& GetAssetsDirectory();

    std::filesystem::path GetLogFilePath();
    std::filesystem::path MakePathRelativeToExecutable(const std::filesystem::path& relativePath);

    bool EnsureDirectoryExists(const std::filesystem::path& directoryPath);
    bool Exists(const std::filesystem::path& path);
    bool IsDirectory(const std::filesystem::path& path);
    std::string ToString(const std::filesystem::path& path);
}