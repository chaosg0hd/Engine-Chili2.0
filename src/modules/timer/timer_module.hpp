#pragma once

#include "../../core/module.hpp"

class EngineContext;

class TimerModule : public IModule
{
public:
    TimerModule();

    const char* GetName() const override;

    bool Initialize(EngineContext& context) override;
    void Startup(EngineContext& context) override;
    void Update(EngineContext& context, float deltaTime) override;
    void Shutdown(EngineContext& context) override;

    double GetDeltaTime() const;
    double GetTotalTime() const;

    bool IsInitialized() const;
    bool IsStarted() const;

private:
    void Reset();
    void Tick();

    static long long GetCurrentTicks();
    static double TicksToSeconds(long long ticks);

private:
    bool m_initialized = false;
    bool m_started = false;

    double m_deltaTime = 0.0;
    double m_totalTime = 0.0;

    long long m_startTicks = 0;
    long long m_lastTicks = 0;
};