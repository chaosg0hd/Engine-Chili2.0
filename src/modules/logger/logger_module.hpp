#pragma once

#include "../../core/module.hpp"

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

    bool IsInitialized() const;
    bool IsStarted() const;

private:
    bool m_initialized = false;
    bool m_started = false;
};