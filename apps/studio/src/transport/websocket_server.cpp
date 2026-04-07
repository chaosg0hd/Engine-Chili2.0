#include "websocket_server.hpp"

#include "../bridge/engine_bridge.hpp"

#include <winsock2.h>
#include <ws2tcpip.h>

#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace
{
    constexpr int kReceiveBufferSize = 4096;
    constexpr const char* kWebSocketGuid = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
    constexpr const char* kProtocolVersion = "1";

    struct WebSocketClientConnection
    {
        SOCKET socket = INVALID_SOCKET;
        std::string receiveBuffer;
        bool handshakeComplete = false;
    };

    bool SetNonBlocking(SOCKET socket)
    {
        u_long enabled = 1;
        return ioctlsocket(socket, FIONBIO, &enabled) == 0;
    }

    std::string EscapeJson(const std::string& value)
    {
        std::string escaped;
        escaped.reserve(value.size());

        for (char ch : value)
        {
            if (ch == '\\' || ch == '"')
            {
                escaped.push_back('\\');
            }

            escaped.push_back(ch);
        }

        return escaped;
    }

    std::string ExtractHeaderValue(const std::string& request, const std::string& headerName)
    {
        const std::string pattern = headerName + ":";
        std::size_t start = request.find(pattern);
        if (start == std::string::npos)
        {
            return std::string();
        }

        start += pattern.size();
        while (start < request.size() && (request[start] == ' ' || request[start] == '\t'))
        {
            ++start;
        }

        const std::size_t end = request.find("\r\n", start);
        if (end == std::string::npos)
        {
            return std::string();
        }

        return request.substr(start, end - start);
    }

    std::string Base64Encode(const unsigned char* data, std::size_t size)
    {
        static constexpr char kAlphabet[] =
            "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

        std::string encoded;
        encoded.reserve(((size + 2U) / 3U) * 4U);

        for (std::size_t index = 0; index < size; index += 3U)
        {
            const unsigned int b0 = data[index];
            const unsigned int b1 = (index + 1U < size) ? data[index + 1U] : 0U;
            const unsigned int b2 = (index + 2U < size) ? data[index + 2U] : 0U;
            const unsigned int triple = (b0 << 16U) | (b1 << 8U) | b2;

            encoded.push_back(kAlphabet[(triple >> 18U) & 0x3FU]);
            encoded.push_back(kAlphabet[(triple >> 12U) & 0x3FU]);
            encoded.push_back((index + 1U < size) ? kAlphabet[(triple >> 6U) & 0x3FU] : '=');
            encoded.push_back((index + 2U < size) ? kAlphabet[triple & 0x3FU] : '=');
        }

        return encoded;
    }

    std::array<unsigned char, 20> Sha1Digest(const std::string& input)
    {
        std::vector<unsigned char> bytes(input.begin(), input.end());
        const std::uint64_t bitLength = static_cast<std::uint64_t>(bytes.size()) * 8ULL;

        bytes.push_back(0x80U);
        while ((bytes.size() % 64U) != 56U)
        {
            bytes.push_back(0x00U);
        }

        for (int shift = 56; shift >= 0; shift -= 8)
        {
            bytes.push_back(static_cast<unsigned char>((bitLength >> shift) & 0xFFU));
        }

        std::uint32_t h0 = 0x67452301U;
        std::uint32_t h1 = 0xEFCDAB89U;
        std::uint32_t h2 = 0x98BADCFEU;
        std::uint32_t h3 = 0x10325476U;
        std::uint32_t h4 = 0xC3D2E1F0U;

        for (std::size_t chunk = 0; chunk < bytes.size(); chunk += 64U)
        {
            std::uint32_t words[80] = {};

            for (int index = 0; index < 16; ++index)
            {
                const std::size_t offset = chunk + static_cast<std::size_t>(index * 4);
                words[index] =
                    (static_cast<std::uint32_t>(bytes[offset]) << 24U) |
                    (static_cast<std::uint32_t>(bytes[offset + 1U]) << 16U) |
                    (static_cast<std::uint32_t>(bytes[offset + 2U]) << 8U) |
                    static_cast<std::uint32_t>(bytes[offset + 3U]);
            }

            for (int index = 16; index < 80; ++index)
            {
                const std::uint32_t value = words[index - 3] ^ words[index - 8] ^ words[index - 14] ^ words[index - 16];
                words[index] = (value << 1U) | (value >> 31U);
            }

            std::uint32_t a = h0;
            std::uint32_t b = h1;
            std::uint32_t c = h2;
            std::uint32_t d = h3;
            std::uint32_t e = h4;

            for (int index = 0; index < 80; ++index)
            {
                std::uint32_t f = 0U;
                std::uint32_t k = 0U;

                if (index < 20)
                {
                    f = (b & c) | ((~b) & d);
                    k = 0x5A827999U;
                }
                else if (index < 40)
                {
                    f = b ^ c ^ d;
                    k = 0x6ED9EBA1U;
                }
                else if (index < 60)
                {
                    f = (b & c) | (b & d) | (c & d);
                    k = 0x8F1BBCDCU;
                }
                else
                {
                    f = b ^ c ^ d;
                    k = 0xCA62C1D6U;
                }

                const std::uint32_t rotatedA = (a << 5U) | (a >> 27U);
                const std::uint32_t rotatedB = (b << 30U) | (b >> 2U);
                const std::uint32_t temp = rotatedA + f + e + k + words[index];
                e = d;
                d = c;
                c = rotatedB;
                b = a;
                a = temp;
            }

            h0 += a;
            h1 += b;
            h2 += c;
            h3 += d;
            h4 += e;
        }

        return {
            static_cast<unsigned char>((h0 >> 24U) & 0xFFU),
            static_cast<unsigned char>((h0 >> 16U) & 0xFFU),
            static_cast<unsigned char>((h0 >> 8U) & 0xFFU),
            static_cast<unsigned char>(h0 & 0xFFU),
            static_cast<unsigned char>((h1 >> 24U) & 0xFFU),
            static_cast<unsigned char>((h1 >> 16U) & 0xFFU),
            static_cast<unsigned char>((h1 >> 8U) & 0xFFU),
            static_cast<unsigned char>(h1 & 0xFFU),
            static_cast<unsigned char>((h2 >> 24U) & 0xFFU),
            static_cast<unsigned char>((h2 >> 16U) & 0xFFU),
            static_cast<unsigned char>((h2 >> 8U) & 0xFFU),
            static_cast<unsigned char>(h2 & 0xFFU),
            static_cast<unsigned char>((h3 >> 24U) & 0xFFU),
            static_cast<unsigned char>((h3 >> 16U) & 0xFFU),
            static_cast<unsigned char>((h3 >> 8U) & 0xFFU),
            static_cast<unsigned char>(h3 & 0xFFU),
            static_cast<unsigned char>((h4 >> 24U) & 0xFFU),
            static_cast<unsigned char>((h4 >> 16U) & 0xFFU),
            static_cast<unsigned char>((h4 >> 8U) & 0xFFU),
            static_cast<unsigned char>(h4 & 0xFFU)
        };
    }

    std::string BuildHandshakeResponse(const std::string& request)
    {
        const std::string key = ExtractHeaderValue(request, "Sec-WebSocket-Key");
        if (key.empty())
        {
            return std::string();
        }

        const auto digest = Sha1Digest(key + kWebSocketGuid);
        const std::string acceptKey = Base64Encode(digest.data(), digest.size());

        return std::string("HTTP/1.1 101 Switching Protocols\r\n") +
            "Upgrade: websocket\r\n"
            "Connection: Upgrade\r\n"
            "Sec-WebSocket-Accept: " +
            acceptKey +
            "\r\n\r\n";
    }

    bool SendAll(SOCKET socket, const char* data, std::size_t size)
    {
        std::size_t sentTotal = 0;
        while (sentTotal < size)
        {
            const int sent = send(socket, data + sentTotal, static_cast<int>(size - sentTotal), 0);
            if (sent == SOCKET_ERROR)
            {
                return false;
            }

            sentTotal += static_cast<std::size_t>(sent);
        }

        return true;
    }

    bool SendTextFrame(SOCKET socket, const std::string& message)
    {
        std::vector<unsigned char> frame;
        frame.reserve(message.size() + 10U);

        frame.push_back(0x81U);

        if (message.size() <= 125U)
        {
            frame.push_back(static_cast<unsigned char>(message.size()));
        }
        else if (message.size() <= 65535U)
        {
            frame.push_back(126U);
            frame.push_back(static_cast<unsigned char>((message.size() >> 8U) & 0xFFU));
            frame.push_back(static_cast<unsigned char>(message.size() & 0xFFU));
        }
        else
        {
            frame.push_back(127U);
            for (int shift = 56; shift >= 0; shift -= 8)
            {
                frame.push_back(static_cast<unsigned char>((static_cast<std::uint64_t>(message.size()) >> shift) & 0xFFU));
            }
        }

        frame.insert(frame.end(), message.begin(), message.end());
        return SendAll(socket, reinterpret_cast<const char*>(frame.data()), frame.size());
    }

    bool TryReadFrame(std::string& buffer, std::string& outMessage, bool& outShouldClose)
    {
        outMessage.clear();
        outShouldClose = false;

        if (buffer.size() < 2U)
        {
            return false;
        }

        const unsigned char byte0 = static_cast<unsigned char>(buffer[0]);
        const unsigned char byte1 = static_cast<unsigned char>(buffer[1]);
        const bool fin = (byte0 & 0x80U) != 0U;
        const unsigned char opcode = static_cast<unsigned char>(byte0 & 0x0FU);
        const bool masked = (byte1 & 0x80U) != 0U;
        std::uint64_t payloadLength = static_cast<std::uint64_t>(byte1 & 0x7FU);
        std::size_t offset = 2U;

        if (!fin || !masked)
        {
            outShouldClose = true;
            return false;
        }

        if (payloadLength == 126U)
        {
            if (buffer.size() < offset + 2U)
            {
                return false;
            }

            payloadLength =
                (static_cast<std::uint64_t>(static_cast<unsigned char>(buffer[offset])) << 8U) |
                static_cast<std::uint64_t>(static_cast<unsigned char>(buffer[offset + 1U]));
            offset += 2U;
        }
        else if (payloadLength == 127U)
        {
            if (buffer.size() < offset + 8U)
            {
                return false;
            }

            payloadLength = 0U;
            for (int index = 0; index < 8; ++index)
            {
                payloadLength = (payloadLength << 8U) |
                    static_cast<std::uint64_t>(static_cast<unsigned char>(buffer[offset + static_cast<std::size_t>(index)]));
            }

            offset += 8U;
        }

        if (buffer.size() < offset + 4U + payloadLength)
        {
            return false;
        }

        const unsigned char maskKey[4] =
        {
            static_cast<unsigned char>(buffer[offset]),
            static_cast<unsigned char>(buffer[offset + 1U]),
            static_cast<unsigned char>(buffer[offset + 2U]),
            static_cast<unsigned char>(buffer[offset + 3U])
        };
        offset += 4U;

        if (opcode == 0x8U)
        {
            buffer.erase(0, offset + static_cast<std::size_t>(payloadLength));
            outShouldClose = true;
            return false;
        }

        if (opcode != 0x1U)
        {
            buffer.erase(0, offset + static_cast<std::size_t>(payloadLength));
            return false;
        }

        outMessage.resize(static_cast<std::size_t>(payloadLength));
        for (std::size_t index = 0; index < static_cast<std::size_t>(payloadLength); ++index)
        {
            outMessage[index] = static_cast<char>(
                static_cast<unsigned char>(buffer[offset + index]) ^ maskKey[index % 4U]);
        }

        buffer.erase(0, offset + static_cast<std::size_t>(payloadLength));
        return true;
    }
}

struct WebSocketServerState
{
    SOCKET listener = INVALID_SOCKET;
    std::vector<WebSocketClientConnection> clients;
    bool wsaStarted = false;
};

WebSocketServer::WebSocketServer()
    : m_state(std::make_unique<WebSocketServerState>())
{
}

WebSocketServer::~WebSocketServer() = default;

bool WebSocketServer::Start(const WebSocketServerConfig& config, EngineBridge& bridge)
{
    if (m_running)
    {
        return true;
    }

    if (config.host.empty())
    {
        bridge.LogError("Studio: WebSocket server host is empty.");
        return false;
    }

    m_url = "ws://" + config.host + ":" + std::to_string(config.port);

    WSADATA wsaData = {};
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        bridge.LogError("Studio: WebSocket server failed to initialize Winsock.");
        return false;
    }

    m_state->wsaStarted = true;
    m_state->listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (m_state->listener == INVALID_SOCKET)
    {
        bridge.LogError("Studio: WebSocket server failed to create listener socket.");
        Stop(bridge);
        return false;
    }

    if (!SetNonBlocking(m_state->listener))
    {
        bridge.LogError("Studio: WebSocket server failed to set non-blocking mode.");
        Stop(bridge);
        return false;
    }

    sockaddr_in address = {};
    address.sin_family = AF_INET;
    address.sin_port = htons(config.port);
    address.sin_addr.s_addr = inet_addr(config.host.c_str());

    if (bind(m_state->listener, reinterpret_cast<const sockaddr*>(&address), sizeof(address)) == SOCKET_ERROR)
    {
        bridge.LogError("Studio: WebSocket server failed to bind listener socket.");
        Stop(bridge);
        return false;
    }

    if (listen(m_state->listener, SOMAXCONN) == SOCKET_ERROR)
    {
        bridge.LogError("Studio: WebSocket server failed to listen on localhost.");
        Stop(bridge);
        return false;
    }

    m_running = true;

    bridge.LogInfo(
        std::string("Studio: WebSocket server bound to ") +
        m_url);

    return true;
}

void WebSocketServer::Stop(EngineBridge& bridge)
{
    if (m_state)
    {
        for (WebSocketClientConnection& client : m_state->clients)
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

    bridge.LogInfo("Studio: WebSocket server stopped.");
    m_pendingBroadcasts.clear();
    m_running = false;
}

void WebSocketServer::Tick(EngineBridge& bridge)
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
                bridge.LogWarn("Studio: WebSocket server accept failed.");
            }

            break;
        }

        if (!SetNonBlocking(clientSocket))
        {
            closesocket(clientSocket);
            bridge.LogWarn("Studio: WebSocket server rejected client after non-blocking setup failure.");
            continue;
        }

        m_state->clients.push_back({ clientSocket, std::string(), false });
    }

    std::vector<WebSocketClientConnection> activeClients;
    activeClients.reserve(m_state->clients.size());

    for (WebSocketClientConnection& client : m_state->clients)
    {
        bool keepClient = true;
        char buffer[kReceiveBufferSize] = {};

        for (;;)
        {
            const int received = recv(client.socket, buffer, sizeof(buffer), 0);
            if (received == 0)
            {
                keepClient = false;
                break;
            }

            if (received == SOCKET_ERROR)
            {
                const int error = WSAGetLastError();
                if (error != WSAEWOULDBLOCK)
                {
                    keepClient = false;
                }

                break;
            }

            client.receiveBuffer.append(buffer, static_cast<std::size_t>(received));

            if (!client.handshakeComplete)
            {
                const std::size_t headerEnd = client.receiveBuffer.find("\r\n\r\n");
                if (headerEnd == std::string::npos)
                {
                    continue;
                }

                const std::string handshakeResponse = BuildHandshakeResponse(client.receiveBuffer);
                if (handshakeResponse.empty() || !SendAll(client.socket, handshakeResponse.data(), handshakeResponse.size()))
                {
                    keepClient = false;
                    break;
                }

                client.receiveBuffer.erase(0, headerEnd + 4U);
                client.handshakeComplete = true;
                bridge.LogInfo("Studio: WebSocket client connected.");
            }

            while (client.handshakeComplete)
            {
                std::string incomingMessage;
                bool shouldClose = false;
                if (!TryReadFrame(client.receiveBuffer, incomingMessage, shouldClose))
                {
                    if (shouldClose)
                    {
                        keepClient = false;
                    }

                    break;
                }

                if (!m_handler)
                {
                    continue;
                }

                bridge.LogInfo(
                    std::string("Studio: WebSocket message -> ") +
                    incomingMessage);

                const std::string response = m_handler(incomingMessage);
                if (!response.empty() && !SendTextFrame(client.socket, response))
                {
                    keepClient = false;
                    break;
                }
            }
        }

        if (!keepClient)
        {
            closesocket(client.socket);
            bridge.LogInfo("Studio: WebSocket client disconnected.");
            continue;
        }

        activeClients.push_back(std::move(client));
    }

    m_state->clients = std::move(activeClients);

    if (!m_pendingBroadcasts.empty())
    {
        std::vector<std::string> pendingMessages = std::move(m_pendingBroadcasts);
        m_pendingBroadcasts.clear();

        for (const std::string& message : pendingMessages)
        {
            bridge.LogInfo(
                std::string("Studio: outbound event -> ") +
                message);

            std::vector<WebSocketClientConnection> broadcastClients;
            broadcastClients.reserve(m_state->clients.size());

            for (WebSocketClientConnection& client : m_state->clients)
            {
                if (!client.handshakeComplete || !SendTextFrame(client.socket, message))
                {
                    closesocket(client.socket);
                    continue;
                }

                broadcastClients.push_back(std::move(client));
            }

            m_state->clients = std::move(broadcastClients);
        }
    }
}

void WebSocketServer::SetMessageHandler(MessageHandler handler)
{
    m_handler = std::move(handler);
}

std::string WebSocketServer::DispatchLocalMessage(const std::string& message, EngineBridge& bridge)
{
    if (!m_running)
    {
        bridge.LogWarn("Studio: cannot dispatch local WebSocket message while server is stopped.");
        return std::string();
    }

    if (!m_handler)
    {
        bridge.LogWarn("Studio: no WebSocket message handler is registered.");
        return std::string();
    }

    bridge.LogInfo(
        std::string("Studio: local WebSocket message -> ") +
        message);
    return m_handler(message);
}

void WebSocketServer::Broadcast(const std::string& message)
{
    if (!m_running)
    {
        return;
    }

    m_pendingBroadcasts.push_back(message);
}

void WebSocketServer::BroadcastEvent(const std::string& eventName, const std::string& message)
{
    Broadcast(
        std::string("{\"kind\":\"event\",\"protocol_version\":\"") +
        kProtocolVersion +
        "\",\"event\":\"" +
        EscapeJson(eventName) +
        "\",\"message\":\"" +
        EscapeJson(message) +
        "\"}");
}

bool WebSocketServer::IsRunning() const
{
    return m_running;
}

const std::string& WebSocketServer::GetUrl() const
{
    return m_url;
}
