#pragma once

#include <string>
#include <vector>

namespace studio
{
    struct FileEntry
    {
        std::string name;
        std::string logicalPath;
        bool isDirectory = false;
    };

    class FileProxy
    {
    public:
        explicit FileProxy(std::string logicalRoot = "User");

        bool EnsureWorkspaceFolders(std::string& outError) const;

        std::string Resolve(const std::string& virtualPath) const;
        bool Exists(const std::string& virtualPath) const;
        bool CreateDirectory(const std::string& virtualPath, std::string& outError) const;
        bool WriteText(const std::string& virtualPath, const std::string& text, std::string& outError) const;
        bool ReadText(const std::string& virtualPath, std::string& outText, std::string& outError) const;
        std::vector<FileEntry> ListDirectory(const std::string& virtualPath) const;
        bool ListDirectory(const std::string& virtualPath, std::vector<FileEntry>& outEntries, std::string& outError) const;
        bool IsFile(const std::string& virtualPath) const;
        bool IsDirectory(const std::string& virtualPath) const;
        bool Move(const std::string& fromVirtualPath, const std::string& toVirtualPath, std::string& outError) const;
        bool Rename(const std::string& fromVirtualPath, const std::string& toVirtualPath, std::string& outError) const;

        bool IsValidVirtualPath(const std::string& virtualPath) const;
        std::string NormalizeVirtualPath(const std::string& virtualPath) const;
        const std::string& GetLogicalRoot() const;

    private:
        std::string m_logicalRoot;
    };
}
