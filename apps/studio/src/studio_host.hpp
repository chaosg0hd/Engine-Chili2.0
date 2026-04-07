#pragma once

#include "bridge/engine_bridge.hpp"
#include "commands/command_router.hpp"
#include "events/event_bus.hpp"
#include "network/http_server.hpp"
#include "network/websocket_server.hpp"

class StudioHost
{
public:
    bool Initialize();
    void Run();
    void Shutdown();

private:
    void LogStudioShellStatus();

private:
    EngineBridge m_bridge;
    HttpServer m_httpServer;
    WebSocketServer m_webSocketServer;
    CommandRouter m_commandRouter;
    EventBus m_eventBus;
    bool m_initialized = false;
};
