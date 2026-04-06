#pragma once

#include "../../core/module.hpp"

class EngineContext;

class DiagnosticsModule : public IModule
{
public:
    DiagnosticsModule();

    const char* GetName() const override;

    bool Initialize(EngineContext& context) override;
    void Startup(EngineContext& context) override;
    void Update(EngineContext& context, float deltaTime) override;
    void Shutdown(EngineContext& context) override;

    void EmitReport() const;

    double GetUptimeSeconds() const;
    unsigned long long GetLoopCount() const;

    bool IsInitialized() const;
    bool IsStarted() const;

private:
    bool m_initialized = false;
    bool m_started = false;

    double m_uptimeSeconds = 0.0;
    unsigned long long m_loopCount = 0;
};