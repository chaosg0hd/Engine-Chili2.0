#include "logger_module.hpp"

#include "../../core/engine_context.hpp"

#include <chrono>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <sstream>

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

    const std::filesystem::path logsDirectory("logs");
    std::error_code errorCode;
    std::filesystem::create_directories(logsDirectory, errorCode);

    m_logFilePath = (logsDirectory / "engine_runtime.log").string();
    m_logStream.open(m_logFilePath, std::ios::out | std::ios::app);

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
    WriteLine("INFO", "Logger startup complete.");
}

void LoggerModule::Shutdown(EngineContext& context)
{
    (void)context;

    if (!m_initialized)
    {
        return;
    }

    WriteLine("INFO", "Logger shutdown.");
    if (m_logStream.is_open())
    {
        m_logStream.flush();
        m_logStream.close();
    }

    m_started = false;
    m_initialized = false;
    m_logFilePath.clear();
}

void LoggerModule::Info(const std::string& message)
{
    WriteLine("INFO", message);
}

void LoggerModule::Warn(const std::string& message)
{
    WriteLine("WARN", message);
}

void LoggerModule::Error(const std::string& message)
{
    WriteLine("ERROR", message);
}

const std::string& LoggerModule::GetLogFilePath() const
{
    return m_logFilePath;
}

bool LoggerModule::IsFileLoggingAvailable() const
{
    return m_logStream.is_open();
}

bool LoggerModule::IsInitialized() const
{
    return m_initialized;
}

bool LoggerModule::IsStarted() const
{
    return m_started;
}

void LoggerModule::WriteLine(const char* level, const std::string& message)
{
    const std::string line = "[" + BuildTimestamp() + "][" + level + "] " + message;

    std::lock_guard<std::mutex> lock(m_writeMutex);

    if (std::strcmp(level, "ERROR") == 0)
    {
        std::cerr << line << '\n';
    }
    else
    {
        std::cout << line << '\n';
    }

    if (m_logStream.is_open())
    {
        m_logStream << line << '\n';
        m_logStream.flush();
    }
}

std::string LoggerModule::BuildTimestamp()
{
    const auto now = std::chrono::system_clock::now();
    const std::time_t timeValue = std::chrono::system_clock::to_time_t(now);

    std::tm localTime{};
#ifdef _WIN32
    localtime_s(&localTime, &timeValue);
#else
    localtime_r(&timeValue, &localTime);
#endif

    std::ostringstream stream;
    stream << std::put_time(&localTime, "%Y-%m-%d %H:%M:%S");
    return stream.str();
}
