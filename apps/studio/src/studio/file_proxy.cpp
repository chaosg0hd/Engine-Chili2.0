#include "studio/file_proxy.hpp"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <vector>

namespace studio
{
    namespace
    {
        std::string ReplaceSlashes(std::string path)
        {
            for (char& character : path)
            {
                if (character == '\\')
                {
                    character = '/';
                }
            }

            return path;
        }

        bool HasDrivePrefix(const std::string& path)
        {
            return path.size() >= 2U && path[1] == ':';
        }

        std::vector<std::string> SplitPath(const std::string& path)
        {
            std::vector<std::string> segments;
            std::stringstream stream(path);
            std::string segment;

            while (std::getline(stream, segment, '/'))
            {
                segments.push_back(segment);
            }

            return segments;
        }
    }

    FileProxy::FileProxy(std::string logicalRoot)
        : m_logicalRoot(NormalizeVirtualPath(logicalRoot))
    {
        if (m_logicalRoot.empty() || !IsValidVirtualPath(m_logicalRoot))
        {
            m_logicalRoot = "User";
        }
    }

    bool FileProxy::EnsureWorkspaceFolders(std::string& outError) const
    {
        return CreateDirectory(std::string(), outError);
    }

    std::string FileProxy::Resolve(const std::string& virtualPath) const
    {
        if (!IsValidVirtualPath(virtualPath))
        {
            return std::string();
        }

        const std::string normalizedPath = NormalizeVirtualPath(virtualPath);
        if (!IsValidVirtualPath(normalizedPath))
        {
            return std::string();
        }

        if (normalizedPath.empty())
        {
            return m_logicalRoot;
        }

        return m_logicalRoot + "/" + normalizedPath;
    }

    bool FileProxy::Exists(const std::string& virtualPath) const
    {
        const std::string resolvedPath = Resolve(virtualPath);
        if (resolvedPath.empty())
        {
            return false;
        }

        std::error_code error;
        return std::filesystem::exists(std::filesystem::path(resolvedPath), error) && !error;
    }

    bool FileProxy::CreateDirectory(const std::string& virtualPath, std::string& outError) const
    {
        const std::string resolvedPath = Resolve(virtualPath);
        if (resolvedPath.empty())
        {
            outError = "Invalid workspace path: " + virtualPath;
            return false;
        }

        std::error_code error;
        std::filesystem::create_directories(std::filesystem::path(resolvedPath), error);
        if (error)
        {
            outError = "Failed to create directory '" + virtualPath + "': " + error.message();
            return false;
        }

        return true;
    }

    bool FileProxy::WriteText(const std::string& virtualPath, const std::string& text, std::string& outError) const
    {
        const std::string resolvedPath = Resolve(virtualPath);
        if (resolvedPath.empty())
        {
            outError = "Invalid workspace path: " + virtualPath;
            return false;
        }

        const std::filesystem::path outputPath(resolvedPath);
        std::error_code error;
        std::filesystem::create_directories(outputPath.parent_path(), error);
        if (error)
        {
            outError = "Failed to create parent directory for '" + virtualPath + "': " + error.message();
            return false;
        }

        std::ofstream file(outputPath, std::ios::binary | std::ios::trunc);
        if (!file)
        {
            outError = "Failed to open file for writing: " + virtualPath;
            return false;
        }

        file << text;
        if (!file)
        {
            outError = "Failed to write file: " + virtualPath;
            return false;
        }

        return true;
    }

    bool FileProxy::ReadText(const std::string& virtualPath, std::string& outText, std::string& outError) const
    {
        const std::string resolvedPath = Resolve(virtualPath);
        if (resolvedPath.empty())
        {
            outError = "Invalid workspace path: " + virtualPath;
            return false;
        }

        std::ifstream file(std::filesystem::path(resolvedPath), std::ios::binary);
        if (!file)
        {
            outError = "Failed to open file for reading: " + virtualPath;
            return false;
        }

        std::ostringstream content;
        content << file.rdbuf();
        outText = content.str();
        return true;
    }

    std::vector<FileEntry> FileProxy::ListDirectory(const std::string& virtualPath) const
    {
        std::vector<FileEntry> entries;
        std::string error;
        ListDirectory(virtualPath, entries, error);
        return entries;
    }

    bool FileProxy::ListDirectory(const std::string& virtualPath, std::vector<FileEntry>& outEntries, std::string& outError) const
    {
        outEntries.clear();

        const std::string normalizedPath = NormalizeVirtualPath(virtualPath);
        const std::string resolvedPath = Resolve(normalizedPath);
        if (resolvedPath.empty())
        {
            outError = "Invalid workspace path: " + virtualPath;
            return false;
        }

        std::error_code error;
        const std::filesystem::path directoryPath(resolvedPath);
        if (!std::filesystem::is_directory(directoryPath, error) || error)
        {
            outError = "Directory is not available: " + virtualPath;
            return false;
        }

        for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(directoryPath, error))
        {
            if (error)
            {
                outError = "Failed to list directory '" + virtualPath + "': " + error.message();
                return false;
            }

            FileEntry fileEntry;
            fileEntry.name = entry.path().filename().string();
            fileEntry.logicalPath = normalizedPath.empty() ? fileEntry.name : (normalizedPath + "/" + fileEntry.name);
            fileEntry.isDirectory = entry.is_directory(error) && !error;
            outEntries.push_back(fileEntry);
        }

        if (error)
        {
            outError = "Failed to list directory '" + virtualPath + "': " + error.message();
            return false;
        }

        return true;
    }

    bool FileProxy::IsDirectory(const std::string& virtualPath) const
    {
        const std::string resolvedPath = Resolve(virtualPath);
        if (resolvedPath.empty())
        {
            return false;
        }

        std::error_code error;
        return std::filesystem::is_directory(std::filesystem::path(resolvedPath), error) && !error;
    }

    bool FileProxy::IsFile(const std::string& virtualPath) const
    {
        const std::string resolvedPath = Resolve(virtualPath);
        if (resolvedPath.empty())
        {
            return false;
        }

        std::error_code error;
        return std::filesystem::is_regular_file(std::filesystem::path(resolvedPath), error) && !error;
    }

    bool FileProxy::Move(const std::string& fromVirtualPath, const std::string& toVirtualPath, std::string& outError) const
    {
        const std::string normalizedFromPath = NormalizeVirtualPath(fromVirtualPath);
        const std::string normalizedToPath = NormalizeVirtualPath(toVirtualPath);
        const std::string resolvedFromPath = Resolve(normalizedFromPath);
        const std::string resolvedToPath = Resolve(normalizedToPath);
        if (resolvedFromPath.empty())
        {
            outError = "Invalid source workspace path: " + fromVirtualPath;
            return false;
        }

        if (resolvedToPath.empty())
        {
            outError = "Invalid target workspace path: " + toVirtualPath;
            return false;
        }

        std::error_code error;
        if (!std::filesystem::exists(std::filesystem::path(resolvedFromPath), error) || error)
        {
            outError = "Source path does not exist: " + fromVirtualPath;
            return false;
        }

        const bool targetExists = std::filesystem::exists(std::filesystem::path(resolvedToPath), error);
        if (error)
        {
            outError = "Failed to check target path '" + toVirtualPath + "': " + error.message();
            return false;
        }

        if (targetExists)
        {
            outError = "Target path already exists: " + toVirtualPath;
            return false;
        }

        std::filesystem::create_directories(std::filesystem::path(resolvedToPath).parent_path(), error);
        if (error)
        {
            outError = "Failed to create target parent directory: " + error.message();
            return false;
        }

        std::filesystem::rename(std::filesystem::path(resolvedFromPath), std::filesystem::path(resolvedToPath), error);
        if (error)
        {
            outError = "Failed to move '" + fromVirtualPath + "' to '" + toVirtualPath + "': " + error.message();
            return false;
        }

        return true;
    }

    bool FileProxy::Rename(const std::string& fromVirtualPath, const std::string& toVirtualPath, std::string& outError) const
    {
        return Move(fromVirtualPath, toVirtualPath, outError);
    }

    bool FileProxy::IsValidVirtualPath(const std::string& virtualPath) const
    {
        const std::string normalizedPath = ReplaceSlashes(virtualPath);
        if (HasDrivePrefix(normalizedPath))
        {
            return false;
        }

        if (!normalizedPath.empty() && normalizedPath.front() == '/')
        {
            return false;
        }

        for (const std::string& segment : SplitPath(normalizedPath))
        {
            if (segment == "..")
            {
                return false;
            }
        }

        return true;
    }

    std::string FileProxy::NormalizeVirtualPath(const std::string& virtualPath) const
    {
        const std::string slashedPath = ReplaceSlashes(virtualPath);
        std::vector<std::string> outputSegments;

        for (const std::string& segment : SplitPath(slashedPath))
        {
            if (segment.empty() || segment == ".")
            {
                continue;
            }

            if (segment == "..")
            {
                return std::string();
            }

            outputSegments.push_back(segment);
        }

        std::string normalizedPath;
        for (const std::string& segment : outputSegments)
        {
            if (!normalizedPath.empty())
            {
                normalizedPath += "/";
            }

            normalizedPath += segment;
        }

        return normalizedPath;
    }

    const std::string& FileProxy::GetLogicalRoot() const
    {
        return m_logicalRoot;
    }
}
