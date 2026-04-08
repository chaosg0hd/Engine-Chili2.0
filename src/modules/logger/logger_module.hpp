#pragma once

#include "../../core/module.hpp"

#include <fstream>
#include <mutex>
#include <string>

class EngineContext;

class LoggerModule : public IModule
{
public:
    const char* GetName() const override;

    bool Initialize(EngineContext& context) override;
    void Startup(EngineContext& context) override;
    void Shutdown(EngineContext& context) override;

    void Info(const std::string& message);
    void Warn(const std::string& message);
    void Error(const std::string& message);

    const std::string& GetLogFilePath() const;
    bool IsFileLoggingAvailable() const;

    bool IsInitialized() const;
    bool IsStarted() const;

private:
    void WriteLine(const char* level, const std::string& message);
    static std::string BuildTimestamp();

private:
    bool m_initialized = false;
    bool m_started = false;
    std::string m_logFilePath;
    std::ofstream m_logStream;
    mutable std::mutex m_writeMutex;
};
