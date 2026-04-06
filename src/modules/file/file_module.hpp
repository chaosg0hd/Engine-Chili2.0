#pragma once

#include "../../core/module.hpp"

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

class EngineContext;

class FileModule : public IModule
{
public:
    const char* GetName() const override;

    bool Initialize(EngineContext& context) override;
    void Startup(EngineContext& context) override;
    void Update(EngineContext& context, float deltaTime) override;
    void Shutdown(EngineContext& context) override;

    bool FileExists(const std::string& path) const;
    bool DirectoryExists(const std::string& path) const;
    bool CreateDirectory(const std::string& path);
    bool WriteTextFile(const std::string& path, const std::string& content);
    bool ReadTextFile(const std::string& path, std::string& outContent) const;
    bool WriteBinaryFile(const std::string& path, const std::vector<std::byte>& content);
    bool ReadBinaryFile(const std::string& path, std::vector<std::byte>& outContent) const;
    bool DeleteFile(const std::string& path);
    std::uintmax_t GetFileSize(const std::string& path) const;
    std::string GetWorkingDirectory() const;
    std::vector<std::string> ListDirectory(const std::string& path) const;
    std::string GetAbsolutePath(const std::string& path) const;
    std::string NormalizePath(const std::string& path) const;

    bool IsInitialized() const;
    bool IsStarted() const;

private:
    bool EnsureParentDirectoryExists(const std::string& path) const;

private:
    bool m_initialized = false;
    bool m_started = false;
};
