#pragma once

#include "studio/file_proxy.hpp"

#include <string>
#include <vector>

namespace studio
{
    struct ProxyLibraryEntry
    {
        std::string id;
        std::string type;
        std::string name;
        std::string sourcePath;
        std::string libraryRelativePath;
        std::string version;
        std::vector<std::string> tags;
    };

    struct ProxyLibraryScanResult
    {
        bool success = false;
        std::string message;
        std::string proxyFolderPath;
        std::vector<ProxyLibraryEntry> entries;
    };

    struct ProxyPrototypeInfo
    {
        bool found = false;
        std::string id;
        std::string name;
        std::string type;
        std::string meshAsset;
        std::string materialAsset;
        std::string lightAsset;
        std::string sourcePath;
        std::vector<std::string> components;
    };

    class ProxyLibrary
    {
    public:
        explicit ProxyLibrary(FileProxy files = FileProxy{});

        ProxyLibraryScanResult ScanProjectLibrary(
            const std::string& projectRootPath,
            const std::string& assetProxyFolder) const;

        bool WriteProjectRegistry(
            const std::string& projectRootPath,
            const std::vector<ProxyLibraryEntry>& entries,
            std::string& outError) const;

        bool GetPrototypeInfoById(
            const std::string& proxyFolderPath,
            const std::string& prototypeId,
            ProxyPrototypeInfo& outInfo) const;

    private:
        static std::string NormalizeSlashes(std::string value);
        static std::string EscapeJson(const std::string& value);
        static std::string BuildRegistryJson(const std::vector<ProxyLibraryEntry>& entries);
        static std::string ExtractJsonStringField(const std::string& text, const std::string& fieldName);
        static std::vector<std::string> ParseTags(const std::string& text);
        static std::string GuessTypeFromFolder(const std::string& folderName);
        static std::string GuessTypeFromExtension(const std::string& extension);

        static void EnsureProxyFolderStructure(const std::string& proxyFolderPath);
        static void EnsureBootstrapLibraryContent(const std::string& proxyFolderPath);
        static void ScanFolderEntries(
            const std::string& proxyFolderPath,
            const std::string& folderName,
            std::vector<ProxyLibraryEntry>& outEntries);
        static ProxyLibraryEntry BuildEntryFromPrototypeFile(
            const std::string& filePath,
            const std::string& libraryRelativePath);
        static ProxyLibraryEntry BuildEntryFromGenericFile(
            const std::string& filePath,
            const std::string& libraryRelativePath,
            const std::string& type);

    private:
        FileProxy m_files;
    };
}
