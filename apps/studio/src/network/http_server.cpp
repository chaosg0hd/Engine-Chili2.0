#include "http_server.hpp"

#include "../bridge/engine_bridge.hpp"

#include <winsock2.h>
#include <ws2tcpip.h>

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iterator>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace
{
    constexpr int kReceiveBufferSize = 4096;

    struct HttpClientConnection
    {
        SOCKET socket = INVALID_SOCKET;
        std::string requestBuffer;
    };

    std::string ToLower(std::string value)
    {
        std::transform(
            value.begin(),
            value.end(),
            value.begin(),
            [](unsigned char ch)
            {
                return static_cast<char>(std::tolower(ch));
            });

        return value;
    }

    bool SetNonBlocking(SOCKET socket)
    {
        u_long enabled = 1;
        return ioctlsocket(socket, FIONBIO, &enabled) == 0;
    }

    bool SendAll(SOCKET socket, const std::string& payload)
    {
        std::size_t totalSent = 0;
        while (totalSent < payload.size())
        {
            const int sent = send(
                socket,
                payload.data() + totalSent,
                static_cast<int>(payload.size() - totalSent),
                0);

            if (sent == SOCKET_ERROR)
            {
                return false;
            }

            totalSent += static_cast<std::size_t>(sent);
        }

        return true;
    }

    std::string BuildHttpResponse(
        int statusCode,
        const std::string& statusText,
        const std::string& contentType,
        const std::string& body)
    {
        return std::string("HTTP/1.1 ") +
            std::to_string(statusCode) +
            " " +
            statusText +
            "\r\nContent-Type: " +
            contentType +
            "\r\nContent-Length: " +
            std::to_string(body.size()) +
            "\r\nConnection: close\r\nCache-Control: no-store\r\n\r\n" +
            body;
    }

    std::string GetContentType(const std::string& path)
    {
        const std::size_t extensionPosition = path.find_last_of('.');
        if (extensionPosition == std::string::npos)
        {
            return "application/octet-stream";
        }

        const std::string extension = ToLower(path.substr(extensionPosition));
        if (extension == ".html")
        {
            return "text/html; charset=utf-8";
        }
        if (extension == ".js")
        {
            return "application/javascript; charset=utf-8";
        }
        if (extension == ".css")
        {
            return "text/css; charset=utf-8";
        }
        if (extension == ".json")
        {
            return "application/json; charset=utf-8";
        }
        if (extension == ".txt")
        {
            return "text/plain; charset=utf-8";
        }

        return "application/octet-stream";
    }

    bool ReadFileText(const std::string& path, std::string& outContent)
    {
        std::ifstream input(path, std::ios::binary);
        if (!input)
        {
            outContent.clear();
            return false;
        }

        outContent.assign(
            std::istreambuf_iterator<char>(input),
            std::istreambuf_iterator<char>());
        return true;
    }

    bool BuildResolvedPath(
        const std::string& webRoot,
        const std::string& indexFilePath,
        std::string requestedPath,
        std::string& outResolvedPath)
    {
        if (requestedPath.empty() || requestedPath == "/")
        {
            requestedPath = indexFilePath;
        }

        const std::size_t queryPosition = requestedPath.find('?');
        if (queryPosition != std::string::npos)
        {
            requestedPath = requestedPath.substr(0, queryPosition);
        }

        if (!requestedPath.empty() && requestedPath.front() == '/')
        {
            requestedPath.erase(requestedPath.begin());
        }

        if (requestedPath.find("..") != std::string::npos)
        {
            return false;
        }

        outResolvedPath = webRoot;
        if (!outResolvedPath.empty() && outResolvedPath.back() != '\\' && outResolvedPath.back() != '/')
        {
            outResolvedPath += '/';
        }

        outResolvedPath += requestedPath;
        return true;
    }
}

struct HttpServerState
{
    SOCKET listener = INVALID_SOCKET;
    std::vector<HttpClientConnection> clients;
    bool wsaStarted = false;
};

HttpServer::HttpServer()
    : m_state(std::make_unique<HttpServerState>())
{
}

HttpServer::~HttpServer() = default;

bool HttpServer::Start(const HttpServerConfig& config, EngineBridge& bridge)
{
    if (m_running)
    {
        return true;
    }

    if (config.host.empty() || config.webRootPath.empty())
    {
        bridge.LogError("Studio: HTTP server configuration is incomplete.");
        return false;
    }

    m_baseUrl = "http://" + config.host + ":" + std::to_string(config.port);
    m_webRootPath = config.webRootPath;
    m_indexFilePath = config.indexFilePath.empty() ? std::string("index.html") : config.indexFilePath;

    WSADATA wsaData = {};
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        bridge.LogError("Studio: HTTP server failed to initialize Winsock.");
        return false;
    }

    m_state->wsaStarted = true;
    m_state->listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (m_state->listener == INVALID_SOCKET)
    {
        bridge.LogError("Studio: HTTP server failed to create listener socket.");
        Stop(bridge);
        return false;
    }

    if (!SetNonBlocking(m_state->listener))
    {
        bridge.LogError("Studio: HTTP server failed to set non-blocking mode.");
        Stop(bridge);
        return false;
    }

    sockaddr_in address = {};
    address.sin_family = AF_INET;
    address.sin_port = htons(config.port);
    address.sin_addr.s_addr = inet_addr(config.host.c_str());

    if (bind(m_state->listener, reinterpret_cast<const sockaddr*>(&address), sizeof(address)) == SOCKET_ERROR)
    {
        bridge.LogError("Studio: HTTP server failed to bind listener socket.");
        Stop(bridge);
        return false;
    }

    if (listen(m_state->listener, SOMAXCONN) == SOCKET_ERROR)
    {
        bridge.LogError("Studio: HTTP server failed to listen on localhost.");
        Stop(bridge);
        return false;
    }

    m_running = true;

    bridge.LogInfo(
        std::string("Studio: HTTP server bound to ") +
        m_baseUrl +
        " serving " +
        m_webRootPath);

    return true;
}

void HttpServer::Stop(EngineBridge& bridge)
{
    if (m_state)
    {
        for (HttpClientConnection& client : m_state->clients)
        {
            if (client.socket != INVALID_SOCKET)
            {
                closesocket(client.socket);
                client.socket = INVALID_SOCKET;
            }
        }

        m_state->clients.clear();

        if (m_state->listener != INVALID_SOCKET)
        {
            closesocket(m_state->listener);
            m_state->listener = INVALID_SOCKET;
        }

        if (m_state->wsaStarted)
        {
            WSACleanup();
            m_state->wsaStarted = false;
        }
    }

    if (!m_running)
    {
        return;
    }

    bridge.LogInfo("Studio: HTTP server stopped.");
    m_running = false;
}

void HttpServer::Tick(EngineBridge& bridge)
{
    if (!m_running || !m_state || m_state->listener == INVALID_SOCKET)
    {
        return;
    }

    for (;;)
    {
        SOCKET clientSocket = accept(m_state->listener, nullptr, nullptr);
        if (clientSocket == INVALID_SOCKET)
        {
            const int error = WSAGetLastError();
            if (error != WSAEWOULDBLOCK)
            {
                bridge.LogWarn("Studio: HTTP server accept failed.");
            }

            break;
        }

        if (!SetNonBlocking(clientSocket))
        {
            closesocket(clientSocket);
            bridge.LogWarn("Studio: HTTP server rejected client after non-blocking setup failure.");
            continue;
        }

        m_state->clients.push_back({ clientSocket, std::string() });
    }

    std::vector<HttpClientConnection> activeClients;
    activeClients.reserve(m_state->clients.size());

    for (HttpClientConnection& client : m_state->clients)
    {
        char buffer[kReceiveBufferSize] = {};
        const int received = recv(client.socket, buffer, sizeof(buffer), 0);

        if (received == 0)
        {
            closesocket(client.socket);
            continue;
        }

        if (received == SOCKET_ERROR)
        {
            const int error = WSAGetLastError();
            if (error == WSAEWOULDBLOCK)
            {
                activeClients.push_back(std::move(client));
                continue;
            }

            closesocket(client.socket);
            continue;
        }

        client.requestBuffer.append(buffer, static_cast<std::size_t>(received));

        const std::size_t headerEnd = client.requestBuffer.find("\r\n\r\n");
        if (headerEnd == std::string::npos)
        {
            activeClients.push_back(std::move(client));
            continue;
        }

        const std::size_t requestLineEnd = client.requestBuffer.find("\r\n");
        const std::string requestLine = client.requestBuffer.substr(0, requestLineEnd);
        const std::size_t methodEnd = requestLine.find(' ');
        const std::size_t pathEnd = (methodEnd == std::string::npos) ? std::string::npos : requestLine.find(' ', methodEnd + 1);

        std::string response;
        if (methodEnd == std::string::npos || pathEnd == std::string::npos || requestLine.substr(0, methodEnd) != "GET")
        {
            response = BuildHttpResponse(405, "Method Not Allowed", "text/plain; charset=utf-8", "Only GET is supported.");
        }
        else
        {
            const std::string requestedPath = requestLine.substr(methodEnd + 1, pathEnd - methodEnd - 1);
            std::string resolvedPath;
            if (!BuildResolvedPath(m_webRootPath, m_indexFilePath, requestedPath, resolvedPath))
            {
                response = BuildHttpResponse(403, "Forbidden", "text/plain; charset=utf-8", "Forbidden path.");
            }
            else
            {
                std::string body;
                if (!ReadFileText(resolvedPath, body))
                {
                    response = BuildHttpResponse(404, "Not Found", "text/plain; charset=utf-8", "File not found.");
                }
                else
                {
                    response = BuildHttpResponse(200, "OK", GetContentType(resolvedPath), body);
                }
            }
        }

        SendAll(client.socket, response);
        closesocket(client.socket);
    }

    m_state->clients = std::move(activeClients);
}

bool HttpServer::IsRunning() const
{
    return m_running;
}

const std::string& HttpServer::GetBaseUrl() const
{
    return m_baseUrl;
}

const std::string& HttpServer::GetWebRootPath() const
{
    return m_webRootPath;
}
