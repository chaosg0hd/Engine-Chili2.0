#include "logger_module.hpp"

#include "../../core/engine_context.hpp"

#include <iostream>

const char* LoggerModule::GetName() const
{
    return "Logger";
}

bool LoggerModule::Initialize(EngineContext& context)
{
    (void)context;

    if (m_initialized)
    {
        return true;
    }

    m_initialized = true;
    return true;
}

void LoggerModule::Startup(EngineContext& context)
{
    (void)context;

    if (!m_initialized || m_started)
    {
        return;
    }

    m_started = true;
}

void LoggerModule::Shutdown(EngineContext& context)
{
    (void)context;

    if (!m_initialized)
    {
        return;
    }

    m_started = false;
    m_initialized = false;
}

void LoggerModule::Info(const std::string& message)
{
    std::cout << "[INFO] " << message << '\n';
}

void LoggerModule::Warn(const std::string& message)
{
    std::cout << "[WARN] " << message << '\n';
}

void LoggerModule::Error(const std::string& message)
{
    std::cout << "[ERROR] " << message << '\n';
}

bool LoggerModule::IsInitialized() const
{
    return m_initialized;
}

bool LoggerModule::IsStarted() const
{
    return m_started;
}