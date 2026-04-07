#include "studio_host.hpp"

#include <windows.h>
#include <shellapi.h>

#include <string>

namespace
{
    bool TryOpenStudioBrowser(const std::string& url)
    {
        const std::wstring wideUrl(url.begin(), url.end());
        const HINSTANCE result = ShellExecuteW(
            nullptr,
            L"open",
            wideUrl.c_str(),
            nullptr,
            nullptr,
            SW_SHOWNORMAL);

        return reinterpret_cast<std::intptr_t>(result) > 32;
    }
}

bool StudioHost::Initialize()
{
    if (m_initialized)
    {
        return true;
    }

    if (!m_bridge.Initialize())
    {
        return false;
    }

    HttpServerConfig httpConfig;
    httpConfig.host = "127.0.0.1";
    httpConfig.port = 3000;
    httpConfig.webRootPath = m_bridge.GetStudioWebRootPath();
    httpConfig.indexFilePath = "webhost/index.html";

    if (!m_httpServer.Start(httpConfig, m_bridge))
    {
        m_bridge.LogError("Studio: failed to start local HTTP host.");
        m_bridge.Shutdown();
        return false;
    }

    m_eventBus.Subscribe(
        [this](const std::string& eventName, const std::string& message)
        {
            m_webSocketServer.BroadcastEvent(eventName, message);
        });

    m_commandRouter.Bind(&m_bridge, &m_eventBus);
    m_webSocketServer.SetMessageHandler(
        [this](const std::string& message)
        {
            return m_commandRouter.HandleMessage(message);
        });

    WebSocketServerConfig webSocketConfig;
    webSocketConfig.host = "127.0.0.1";
    webSocketConfig.port = 8765;

    if (!m_webSocketServer.Start(webSocketConfig, m_bridge))
    {
        m_httpServer.Stop(m_bridge);
        m_bridge.LogError("Studio: failed to start local WebSocket host.");
        m_bridge.Shutdown();
        return false;
    }

    LogStudioShellStatus();
    m_eventBus.Publish("studio.started", "Studio host initialized.");
    m_initialized = true;
    return true;
}

void StudioHost::Run()
{
    if (!m_initialized)
    {
        return;
    }

    m_bridge.LogInfo("Studio: browser-first host entering idle loop.");
    m_bridge.LogInfo(
        std::string("Studio: open the frontend at ") +
        m_httpServer.GetBaseUrl() +
        " | websocket = " +
        m_webSocketServer.GetUrl());
    m_bridge.LogInfo("Studio: close the native window or press Escape to stop the browser-connected studio host.");

    if (TryOpenStudioBrowser(m_httpServer.GetBaseUrl()))
    {
        m_bridge.LogInfo("Studio: opened browser for studio webhost.");
    }
    else
    {
        m_bridge.LogWarn("Studio: failed to open the browser automatically. Open the localhost URL manually.");
    }

    while (!m_bridge.ShouldExit())
    {
        if (!m_bridge.Tick())
        {
            break;
        }

        m_httpServer.Tick(m_bridge);
        m_webSocketServer.Tick(m_bridge);
    }
}

void StudioHost::Shutdown()
{
    if (!m_initialized)
    {
        return;
    }

    m_eventBus.Publish("studio.stopping", "Studio host shutting down.");
    m_webSocketServer.Stop(m_bridge);
    m_httpServer.Stop(m_bridge);
    m_bridge.Shutdown();
    m_initialized = false;
}

void StudioHost::LogStudioShellStatus()
{
    m_bridge.LogInfo("Studio: shell boot complete.");
    m_bridge.LogInfo(
        std::string("Studio: web root = ") +
        m_httpServer.GetWebRootPath());
    m_bridge.LogInfo("Studio: hierarchy panel ready.");
    m_bridge.LogInfo("Studio: inspector panel ready.");
    m_bridge.LogInfo("Studio: console panel ready.");
    m_bridge.LogInfo("Studio: toolbar ready.");
}
