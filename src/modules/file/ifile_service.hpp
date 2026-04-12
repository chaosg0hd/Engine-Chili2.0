#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#ifdef CreateDirectory
#undef CreateDirectory
#endif

#ifdef DeleteFile
#undef DeleteFile
#endif

#ifdef CopyFile
#undef CopyFile
#endif

#ifdef MoveFile
#undef MoveFile
#endif

class IFileService
{
public:
    virtual ~IFileService() = default;

    virtual bool FileExists(const std::string& path) const = 0;
    virtual bool DirectoryExists(const std::string& path) const = 0;
    virtual bool CreateDirectory(const std::string& path) = 0;
    virtual bool WriteTextFile(const std::string& path, const std::string& content) = 0;
    virtual bool ReadTextFile(const std::string& path, std::string& outContent) const = 0;
    virtual bool WriteJsonFile(const std::string& path, const std::string& jsonContent) = 0;
    virtual bool ReadJsonFile(const std::string& path, std::string& outJsonContent) const = 0;
    virtual bool WriteBinaryFile(const std::string& path, const std::vector<std::byte>& content) = 0;
    virtual bool ReadBinaryFile(const std::string& path, std::vector<std::byte>& outContent) const = 0;
    virtual bool DeleteFile(const std::string& path) = 0;
    virtual std::uintmax_t GetFileSize(const std::string& path) const = 0;
    virtual std::string GetWorkingDirectory() const = 0;
    virtual std::vector<std::string> ListDirectory(const std::string& path) const = 0;
    virtual std::vector<std::string> ListFiles(const std::string& path) const = 0;
    virtual std::vector<std::string> ListDirectories(const std::string& path) const = 0;
    virtual std::string GetAbsolutePath(const std::string& path) const = 0;
    virtual std::string NormalizePath(const std::string& path) const = 0;
    virtual bool CopyFile(const std::string& source, const std::string& destination) = 0;
    virtual bool MoveFile(const std::string& source, const std::string& destination) = 0;
    virtual bool IsStarted() const = 0;
};
