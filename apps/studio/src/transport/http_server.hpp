#pragma once

#include <memory>
#include <string>

class EngineBridge;
struct HttpServerState;

struct HttpServerConfig
{
    std::string host;
    unsigned short port = 3000;
    std::string webRootPath;
    std::string indexFilePath = "index.html";
};

class HttpServer
{
public:
    HttpServer();
    ~HttpServer();

    bool Start(const HttpServerConfig& config, EngineBridge& bridge);
    void Stop(EngineBridge& bridge);
    void Tick(EngineBridge& bridge);

    bool IsRunning() const;
    const std::string& GetBaseUrl() const;
    const std::string& GetWebRootPath() const;

private:
    bool m_running = false;
    std::string m_baseUrl;
    std::string m_webRootPath;
    std::string m_indexFilePath;
    std::unique_ptr<HttpServerState> m_state;
};
