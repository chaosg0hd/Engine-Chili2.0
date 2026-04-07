#include "command_router.hpp"

#include "../bridge/engine_bridge.hpp"

namespace
{
    constexpr const char* kProtocolVersion = "1";

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
}

void CommandRouter::Bind(EngineBridge* bridge)
{
    m_bridge = bridge;
}

std::string CommandRouter::HandleMessage(const std::string& message)
{
    if (!m_bridge)
    {
        return BuildResponse(false, "unknown", std::string(), "Studio backend is not initialized.", "backend_unavailable");
    }

    CommandEnvelope envelope;
    if (!ParseEnvelope(message, envelope))
    {
        return BuildResponse(false, "unknown", std::string(), "Malformed studio command envelope.", "bad_envelope");
    }

    if (envelope.kind != "command")
    {
        return BuildResponse(false, envelope.command, envelope.requestId, "Only command envelopes are accepted.", "unsupported_kind");
    }

    if (envelope.command.empty())
    {
        return BuildResponse(false, envelope.command, envelope.requestId, "Command field is required.", "missing_command");
    }

    if (envelope.command == "hello")
    {
        const std::string helloMessage = m_bridge->BuildHelloMessage(envelope.sender);
        return BuildResponse(true, envelope.command, envelope.requestId, helloMessage);
    }

    if (envelope.command == "ping")
    {
        return BuildResponse(true, envelope.command, envelope.requestId, "pong");
    }

    if (envelope.command == "get_status")
    {
        return BuildResponse(true, envelope.command, envelope.requestId, m_bridge->BuildStatusMessage());
    }

    if (envelope.command == "exit")
    {
        m_bridge->RequestExit();
        return BuildResponse(true, envelope.command, envelope.requestId, "Studio shutdown requested by frontend.");
    }

    return BuildResponse(false, envelope.command, envelope.requestId, "Unknown studio command.", "unknown_command");
}

bool CommandRouter::ParseEnvelope(const std::string& message, CommandEnvelope& outEnvelope)
{
    outEnvelope.kind = ExtractField(message, "kind");
    outEnvelope.command = ExtractField(message, "command");
    outEnvelope.sender = ExtractField(message, "sender");
    outEnvelope.requestId = ExtractField(message, "request_id");

    const std::string version = ExtractField(message, "protocol_version");
    if (!version.empty() && version != kProtocolVersion)
    {
        return false;
    }

    return !(outEnvelope.kind.empty() && outEnvelope.command.empty() && outEnvelope.sender.empty() && outEnvelope.requestId.empty());
}

std::string CommandRouter::ExtractField(const std::string& message, const std::string& fieldName)
{
    const std::string pattern = "\"" + fieldName + "\"";
    const std::size_t namePosition = message.find(pattern);
    if (namePosition == std::string::npos)
    {
        return std::string();
    }

    const std::size_t colonPosition = message.find(':', namePosition + pattern.size());
    if (colonPosition == std::string::npos)
    {
        return std::string();
    }

    const std::size_t openingQuote = message.find('"', colonPosition + 1);
    if (openingQuote == std::string::npos)
    {
        return std::string();
    }

    const std::size_t closingQuote = message.find('"', openingQuote + 1);
    if (closingQuote == std::string::npos)
    {
        return std::string();
    }

    return message.substr(openingQuote + 1, closingQuote - openingQuote - 1);
}

std::string CommandRouter::BuildResponse(
    bool ok,
    const std::string& command,
    const std::string& requestId,
    const std::string& message,
    const std::string& code)
{
    return std::string("{\"kind\":\"response\",\"protocol_version\":\"") +
        kProtocolVersion +
        "\",\"ok\":" +
        (ok ? "true" : "false") +
        ",\"command\":\"" +
        EscapeJson(command) +
        "\",\"request_id\":\"" +
        EscapeJson(requestId) +
        "\",\"code\":\"" +
        EscapeJson(code) +
        "\",\"message\":\"" +
        EscapeJson(message) +
        "\"}";
}
