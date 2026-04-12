#pragma once

#include "../../core/module.hpp"
#include "ifile_service.hpp"

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

class EngineContext;

class FileModule : public IModule, public IFileService
{
public:
    const char* GetName() const override;

    bool Initialize(EngineContext& context) override;
    void Startup(EngineContext& context) override;
    void Update(EngineContext& context, float deltaTime) override;
    void Shutdown(EngineContext& context) override;

    bool FileExists(const std::string& path) const override;
    bool DirectoryExists(const std::string& path) const override;
    bool CreateDirectory(const std::string& path) override;
    bool WriteTextFile(const std::string& path, const std::string& content) override;
    bool ReadTextFile(const std::string& path, std::string& outContent) const override;
    bool WriteJsonFile(const std::string& path, const std::string& jsonContent) override;
    bool ReadJsonFile(const std::string& path, std::string& outJsonContent) const override;
    bool WriteBinaryFile(const std::string& path, const std::vector<std::byte>& content) override;
    bool ReadBinaryFile(const std::string& path, std::vector<std::byte>& outContent) const override;
    bool DeleteFile(const std::string& path) override;
    std::uintmax_t GetFileSize(const std::string& path) const override;
    std::string GetWorkingDirectory() const override;
    std::vector<std::string> ListDirectory(const std::string& path) const override;
    std::vector<std::string> ListFiles(const std::string& path) const override;
    std::vector<std::string> ListDirectories(const std::string& path) const override;
    std::string GetAbsolutePath(const std::string& path) const override;
    std::string NormalizePath(const std::string& path) const override;
    bool CopyFile(const std::string& source, const std::string& destination) override;
    bool MoveFile(const std::string& source, const std::string& destination) override;

    bool IsInitialized() const;
    bool IsStarted() const override;

private:
    bool EnsureParentDirectoryExists(const std::string& path) const;

private:
    bool m_initialized = false;
    bool m_started = false;
};
