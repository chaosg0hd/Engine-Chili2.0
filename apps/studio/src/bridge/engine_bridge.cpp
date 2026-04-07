#include "engine_bridge.hpp"

namespace
{
    constexpr const char* kStudioWebRoot = "apps/studio";
    constexpr const char* kStudioWebUrl = "http://127.0.0.1:3000";
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

std::string EngineBridge::GetStudioWebRootPath() const
{
    if (!m_initialized)
    {
        return std::string(kStudioWebRoot);
    }

    return m_core.GetAbsolutePath(kStudioWebRoot);
}

std::string EngineBridge::GetStudioWebUrl() const
{
    return std::string(kStudioWebUrl);
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
        " | web_url=" +
        GetStudioWebUrl() +
        " | web_root=" +
        GetStudioWebRootPath();
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
