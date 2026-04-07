#include "engine_bridge.hpp"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>

namespace
{
    constexpr const char* kStudioRoot = "apps/studio";
    constexpr const char* kPackagedCoreToolsEntry = "coretools/runtime/index.html";
    constexpr const char* kSourceCoreToolsEntry = "apps/studio/coretools/runtime/index.html";

    std::string GetExecutableDirectory()
    {
        wchar_t modulePath[MAX_PATH] = {};
        const DWORD length = GetModuleFileNameW(nullptr, modulePath, MAX_PATH);
        if (length == 0 || length >= MAX_PATH)
        {
            return std::string();
        }

        std::wstring path(modulePath, modulePath + length);
        const std::size_t separator = path.find_last_of(L"\\/");
        if (separator == std::wstring::npos)
        {
            return std::string();
        }

        path.resize(separator);
        return std::string(path.begin(), path.end());
    }

    bool FileExists(const std::string& path)
    {
        if (path.empty())
        {
            return false;
        }

        const std::wstring widePath(path.begin(), path.end());
        const DWORD attributes = GetFileAttributesW(widePath.c_str());
        return attributes != INVALID_FILE_ATTRIBUTES &&
            (attributes & FILE_ATTRIBUTE_DIRECTORY) == 0;
    }
}

bool EngineBridge::Initialize()
{
    if (m_initialized)
    {
        return true;
    }

    if (!m_core.Initialize())
    {
        return false;
    }

    m_exitRequested = false;
    m_core.SetWindowTitle(L"Engine Studio");
    m_core.SetWindowOverlayEnabled(false);
    m_initialized = true;
    m_core.LogInfo("Studio: engine bridge initialized.");
    return true;
}

void EngineBridge::Shutdown()
{
    if (!m_initialized)
    {
        return;
    }

    m_core.LogInfo("Studio: engine bridge shutting down.");
    m_core.Shutdown();
    m_initialized = false;
}

void EngineBridge::LogInfo(const std::string& message)
{
    if (m_initialized)
    {
        m_core.LogInfo(message);
    }
}

void EngineBridge::LogWarn(const std::string& message)
{
    if (m_initialized)
    {
        m_core.LogWarn(message);
    }
}

void EngineBridge::LogError(const std::string& message)
{
    if (m_initialized)
    {
        m_core.LogError(message);
    }
}

std::string EngineBridge::GetStudioRootPath() const
{
    if (!m_initialized)
    {
        return std::string(kStudioRoot);
    }

    return m_core.GetAbsolutePath(kStudioRoot);
}

std::string EngineBridge::GetCoreToolsContentPath() const
{
    const std::string executableDirectory = GetExecutableDirectory();
    if (!executableDirectory.empty())
    {
        const std::string packagedPath = executableDirectory + "/" + kPackagedCoreToolsEntry;
        if (FileExists(packagedPath))
        {
            return packagedPath;
        }
    }

    if (!m_initialized)
    {
        return std::string(kSourceCoreToolsEntry);
    }

    const std::string sourcePath = m_core.GetAbsolutePath(kSourceCoreToolsEntry);
    if (FileExists(sourcePath))
    {
        return sourcePath;
    }

    if (!executableDirectory.empty())
    {
        return executableDirectory + "/" + kPackagedCoreToolsEntry;
    }

    return sourcePath;
}

std::string EngineBridge::BuildHelloMessage(const std::string& sender) const
{
    const std::string source = sender.empty() ? "frontend" : sender;
    return "Hello from studio backend. Sender = " + source;
}

std::string EngineBridge::BuildStatusMessage() const
{
    return std::string("studio_initialized=") +
        (m_initialized ? "true" : "false") +
        " | exit_requested=" +
        (m_exitRequested ? "true" : "false") +
        " | native_window=" +
        (GetNativeWindowHandle() ? "ready" : "missing") +
        " | studio_root=" +
        GetStudioRootPath() +
        " | coretools=" +
        GetCoreToolsContentPath();
}

bool EngineBridge::Tick()
{
    if (!m_initialized)
    {
        return false;
    }

    if (!m_core.Tick())
    {
        m_exitRequested = true;
        m_core.LogInfo("Studio: engine tick requested host shutdown.");
        return false;
    }

    return true;
}

EngineCore& EngineBridge::GetCore()
{
    return m_core;
}

const EngineCore& EngineBridge::GetCore() const
{
    return m_core;
}

HWND EngineBridge::GetNativeWindowHandle() const
{
    if (!m_initialized)
    {
        return nullptr;
    }

    return m_core.GetWindowHandle();
}

void EngineBridge::RequestExit()
{
    m_exitRequested = true;

    if (m_initialized)
    {
        m_core.LogInfo("Studio: backend accepted shutdown request.");
        m_core.RequestShutdown();
    }
}

bool EngineBridge::ShouldExit() const
{
    return m_exitRequested;
}

bool EngineBridge::IsInitialized() const
{
    return m_initialized;
}
