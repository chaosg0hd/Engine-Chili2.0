#pragma once

#include <string>

class EngineBridge;

class CommandRouter
{
public:
    void Bind(EngineBridge* bridge);
    std::string HandleMessage(const std::string& message);

private:
    struct CommandEnvelope
    {
        std::string kind;
        std::string command;
        std::string sender;
        std::string requestId;
    };

private:
    static bool ParseEnvelope(const std::string& message, CommandEnvelope& outEnvelope);
    static std::string ExtractField(const std::string& message, const std::string& fieldName);
    static std::string BuildResponse(
        bool ok,
        const std::string& command,
        const std::string& requestId,
        const std::string& message,
        const std::string& code = "ok");

private:
    EngineBridge* m_bridge = nullptr;
};
