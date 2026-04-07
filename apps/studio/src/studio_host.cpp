#include "studio_host.hpp"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>

#include <string>

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

    const HWND windowHandle = m_bridge.GetNativeWindowHandle();
    if (!windowHandle)
    {
        m_bridge.LogError("Studio: native host window is not available.");
        m_bridge.Shutdown();
        return false;
    }

    GetClientRect(windowHandle, &m_lastClientRect);
    const DockLayout::Regions regions = m_dockLayout.Compute(m_lastClientRect);

    if (!m_coreToolsHost.Initialize(windowHandle, regions.leftDock, m_bridge.GetCoreToolsContentPath()))
    {
        m_bridge.LogError("Studio: failed to initialize the embedded CoreTools host.");
        m_bridge.Shutdown();
        return false;
    }

    m_commandRouter.Bind(&m_bridge);
    LogStudioShellStatus();
    m_initialized = true;
    return true;
}

void StudioHost::Run()
{
    if (!m_initialized)
    {
        return;
    }

    m_bridge.LogInfo("Studio: native host entering main loop.");
    m_bridge.LogInfo("Studio: close the native window or press Escape to stop the studio host.");

    while (!m_bridge.ShouldExit())
    {
        if (!m_bridge.Tick())
        {
            break;
        }

        UpdateLayout();
    }
}

void StudioHost::Shutdown()
{
    if (!m_initialized)
    {
        return;
    }

    m_coreToolsHost.Shutdown();
    m_bridge.Shutdown();
    m_initialized = false;
}

void StudioHost::LogStudioShellStatus()
{
    m_bridge.LogInfo("Studio: shell boot complete.");
    m_bridge.LogInfo("Studio: native outer window is active.");
    m_bridge.LogInfo(
        std::string("Studio: CoreTools entry = ") +
        m_bridge.GetCoreToolsContentPath());
    m_bridge.LogInfo(
        std::string("Studio: left dock width = ") +
        std::to_string(m_dockLayout.GetLeftDockWidth()));
    m_bridge.LogInfo("Studio: transport scaffolding is present under src/transport and currently inactive.");
}

void StudioHost::UpdateLayout()
{
    const HWND windowHandle = m_bridge.GetNativeWindowHandle();
    if (!windowHandle)
    {
        return;
    }

    RECT clientRect{};
    GetClientRect(windowHandle, &clientRect);
    if (EqualRect(&clientRect, &m_lastClientRect))
    {
        return;
    }

    m_lastClientRect = clientRect;
    const DockLayout::Regions regions = m_dockLayout.Compute(clientRect);
    m_coreToolsHost.Resize(regions.leftDock);
}
