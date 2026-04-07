#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>

class EngineBridge;
struct WebSocketServerState;

struct WebSocketServerConfig
{
    std::string host;
    unsigned short port = 8765;
};

class WebSocketServer
{
public:
    using MessageHandler = std::function<std::string(const std::string&)>;

    WebSocketServer();
    ~WebSocketServer();

    bool Start(const WebSocketServerConfig& config, EngineBridge& bridge);
    void Stop(EngineBridge& bridge);
    void Tick(EngineBridge& bridge);

    void SetMessageHandler(MessageHandler handler);
    std::string DispatchLocalMessage(const std::string& message, EngineBridge& bridge);
    void Broadcast(const std::string& message);
    void BroadcastEvent(const std::string& eventName, const std::string& message);

    bool IsRunning() const;
    const std::string& GetUrl() const;

private:
    bool m_running = false;
    std::string m_url;
    MessageHandler m_handler;
    std::vector<std::string> m_pendingBroadcasts;
    std::unique_ptr<WebSocketServerState> m_state;
};
