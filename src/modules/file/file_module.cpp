#include "file_module.hpp"

#include "../../core/engine_context.hpp"

#include <filesystem>
#include <fstream>
#include <iterator>

namespace
{
    namespace fs = std::filesystem;
}

const char* FileModule::GetName() const
{
    return "File";
}

bool FileModule::Initialize(EngineContext& context)
{
    (void)context;

    if (m_initialized)
    {
        return true;
    }

    m_initialized = true;
    return true;
}

void FileModule::Startup(EngineContext& context)
{
    (void)context;

    if (!m_initialized || m_started)
    {
        return;
    }

    m_started = true;
}

void FileModule::Update(EngineContext& context, float deltaTime)
{
    (void)context;
    (void)deltaTime;
}

void FileModule::Shutdown(EngineContext& context)
{
    (void)context;

    if (!m_initialized)
    {
        return;
    }

    m_started = false;
    m_initialized = false;
}

bool FileModule::FileExists(const std::string& path) const
{
    std::error_code error;
    return fs::is_regular_file(fs::path(path), error);
}

bool FileModule::DirectoryExists(const std::string& path) const
{
    std::error_code error;
    return fs::is_directory(fs::path(path), error);
}

bool FileModule::CreateDirectory(const std::string& path)
{
    std::error_code error;
    const fs::path directoryPath(path);

    if (directoryPath.empty())
    {
        return false;
    }

    if (fs::exists(directoryPath, error))
    {
        return fs::is_directory(directoryPath, error);
    }

    return fs::create_directories(directoryPath, error);
}

bool FileModule::WriteTextFile(const std::string& path, const std::string& content)
{
    if (!EnsureParentDirectoryExists(path))
    {
        return false;
    }

    std::ofstream output(path, std::ios::out | std::ios::trunc);
    if (!output.is_open())
    {
        return false;
    }

    output.write(content.data(), static_cast<std::streamsize>(content.size()));
    return output.good();
}

bool FileModule::ReadTextFile(const std::string& path, std::string& outContent) const
{
    outContent.clear();

    std::ifstream input(path, std::ios::in);
    if (!input.is_open())
    {
        return false;
    }

    outContent.assign(
        std::istreambuf_iterator<char>(input),
        std::istreambuf_iterator<char>());

    return input.good() || input.eof();
}

bool FileModule::WriteBinaryFile(const std::string& path, const std::vector<std::byte>& content)
{
    if (!EnsureParentDirectoryExists(path))
    {
        return false;
    }

    std::ofstream output(path, std::ios::binary | std::ios::out | std::ios::trunc);
    if (!output.is_open())
    {
        return false;
    }

    if (!content.empty())
    {
        output.write(
            reinterpret_cast<const char*>(content.data()),
            static_cast<std::streamsize>(content.size()));
    }

    return output.good();
}

bool FileModule::ReadBinaryFile(const std::string& path, std::vector<std::byte>& outContent) const
{
    outContent.clear();

    std::ifstream input(path, std::ios::binary | std::ios::in);
    if (!input.is_open())
    {
        return false;
    }

    input.seekg(0, std::ios::end);
    const std::streamoff size = input.tellg();
    if (size < 0)
    {
        return false;
    }

    input.seekg(0, std::ios::beg);
    outContent.resize(static_cast<std::size_t>(size));

    if (!outContent.empty())
    {
        input.read(
            reinterpret_cast<char*>(outContent.data()),
            static_cast<std::streamsize>(outContent.size()));
    }

    return input.good() || input.eof();
}

bool FileModule::DeleteFile(const std::string& path)
{
    std::error_code error;
    return fs::remove(fs::path(path), error);
}

std::uintmax_t FileModule::GetFileSize(const std::string& path) const
{
    std::error_code error;
    const std::uintmax_t size = fs::file_size(fs::path(path), error);
    return error ? 0 : size;
}

std::string FileModule::GetWorkingDirectory() const
{
    std::error_code error;
    const fs::path workingDirectory = fs::current_path(error);
    return error ? std::string() : workingDirectory.string();
}

std::vector<std::string> FileModule::ListDirectory(const std::string& path) const
{
    std::vector<std::string> entries;
    std::error_code error;
    const fs::path directoryPath(path.empty() ? "." : path);

    if (!fs::is_directory(directoryPath, error))
    {
        return entries;
    }

    for (const fs::directory_entry& entry : fs::directory_iterator(directoryPath, error))
    {
        if (error)
        {
            entries.clear();
            break;
        }

        entries.push_back(entry.path().string());
    }

    return entries;
}

std::string FileModule::GetAbsolutePath(const std::string& path) const
{
    std::error_code error;
    const fs::path absolutePath = fs::absolute(fs::path(path), error);
    return error ? std::string() : absolutePath.string();
}

std::string FileModule::NormalizePath(const std::string& path) const
{
    std::error_code error;
    const fs::path normalizedPath = fs::weakly_canonical(fs::path(path), error);

    if (error)
    {
        error.clear();
        const fs::path fallbackPath = fs::path(path).lexically_normal();
        return fallbackPath.string();
    }

    return normalizedPath.string();
}

bool FileModule::IsInitialized() const
{
    return m_initialized;
}

bool FileModule::IsStarted() const
{
    return m_started;
}

bool FileModule::EnsureParentDirectoryExists(const std::string& path) const
{
    std::error_code error;
    const fs::path filePath(path);
    const fs::path parentPath = filePath.parent_path();

    if (parentPath.empty())
    {
        return true;
    }

    if (fs::exists(parentPath, error))
    {
        return fs::is_directory(parentPath, error);
    }

    return fs::create_directories(parentPath, error);
}
