#include "timer_module.hpp"

#include "../../core/engine_context.hpp"

#include <chrono>

namespace
{
    using Clock = std::chrono::high_resolution_clock;
}

TimerModule::TimerModule()
    : m_initialized(false),
      m_started(false),
      m_deltaTime(0.0),
      m_totalTime(0.0),
      m_startTicks(0),
      m_lastTicks(0)
{
}

const char* TimerModule::GetName() const
{
    return "Timer";
}

bool TimerModule::Initialize(EngineContext& context)
{
    (void)context;

    if (m_initialized)
    {
        return true;
    }

    Reset();
    m_initialized = true;
    return true;
}

void TimerModule::Startup(EngineContext& context)
{
    (void)context;

    if (!m_initialized || m_started)
    {
        return;
    }

    Reset();
    m_started = true;
}

void TimerModule::Update(EngineContext& context, float deltaTime)
{
    (void)context;
    (void)deltaTime;

    if (!m_started)
    {
        return;
    }

    Tick();
}

void TimerModule::Shutdown(EngineContext& context)
{
    (void)context;

    if (!m_initialized)
    {
        return;
    }

    m_started = false;
    m_initialized = false;
}

double TimerModule::GetDeltaTime() const
{
    return m_deltaTime;
}

double TimerModule::GetTotalTime() const
{
    return m_totalTime;
}

bool TimerModule::IsInitialized() const
{
    return m_initialized;
}

bool TimerModule::IsStarted() const
{
    return m_started;
}

void TimerModule::Reset()
{
    m_startTicks = GetCurrentTicks();
    m_lastTicks = m_startTicks;
    m_deltaTime = 0.0;
    m_totalTime = 0.0;
}

void TimerModule::Tick()
{
    const long long currentTicks = GetCurrentTicks();

    const long long deltaTicks = currentTicks - m_lastTicks;
    const long long totalTicks = currentTicks - m_startTicks;

    m_deltaTime = TicksToSeconds(deltaTicks);
    m_totalTime = TicksToSeconds(totalTicks);

    m_lastTicks = currentTicks;
}

long long TimerModule::GetCurrentTicks()
{
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
        Clock::now().time_since_epoch()
    ).count();
}

double TimerModule::TicksToSeconds(long long ticks)
{
    return static_cast<double>(ticks) / 1'000'000'000.0;
}