#pragma once

#include "bridge/engine_bridge.hpp"
#include "commands/command_router.hpp"
#include "layout/dock_layout.hpp"
#include "webview/coretools_host.hpp"

class StudioHost
{
public:
    bool Initialize();
    void Run();
    void Shutdown();

private:
    void LogStudioShellStatus();
    void UpdateLayout();

private:
    EngineBridge m_bridge;
    DockLayout m_dockLayout;
    CoreToolsHost m_coreToolsHost;
    CommandRouter m_commandRouter;
    RECT m_lastClientRect{};
    bool m_initialized = false;
};
