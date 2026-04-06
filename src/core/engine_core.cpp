#include "engine_core.hpp"

#include "../modules/logger/logger_module.hpp"
#include "../modules/timer/timer_module.hpp"
#include "../modules/diagnostics/diagnostics_module.hpp"
#include "../modules/platform/platform_module.hpp"
#include "../modules/jobs/job_module.hpp"
#include "../modules/memory/memory_module.hpp"

#include <string>

EngineCore::EngineCore()
    : m_initialized(false),
      m_running(false),
      m_lastWindowOpen(false),
      m_lastWindowActive(false),
      m_nextDiagnosticsLogTime(10.0)
{
}

bool EngineCore::Initialize()
{
    if (m_initialized)
    {
        return true;
    }

    m_logger = m_modules.AddModule<LoggerModule>();
    m_timer = m_modules.AddModule<TimerModule>();
    m_diagnostics = m_modules.AddModule<DiagnosticsModule>();
    m_platform = m_modules.AddModule<PlatformModule>();
    m_jobs = m_modules.AddModule<JobModule>();
    m_memory = m_modules.AddModule<MemoryModule>();

    if (!m_modules.InitializeAll(m_context))
    {
        if (m_logger)
        {
            m_logger->Error("EngineCore initialization failed.");
        }
        return false;
    }

    m_modules.StartupAll(m_context);

    m_lastWindowOpen = (m_platform != nullptr) ? m_platform->IsWindowOpen() : false;
    m_lastWindowActive = (m_platform != nullptr) ? m_platform->IsWindowActive() : false;
    m_nextDiagnosticsLogTime = 10.0;

    if (m_logger)
    {
        m_logger->Info("EngineCore startup complete.");
    }

    m_initialized = true;
    return true;
}

bool EngineCore::Run()
{
    if (!m_initialized)
    {
        return false;
    }

    if (m_running)
    {
        return true;
    }

    m_running = true;
    m_context.IsRunning = true;

    while (m_context.IsRunning)
    {
        BeginFrame();

        ServicePlatform();
        DispatchAsyncWork();
        ServiceCoreWork();
        ProcessEvents();
        ProcessCompletedWork();
        ServiceScheduledWork();
        ServiceDeferredWork();
        SynchronizeCriticalWork();

        EndFrame();
    }

    m_running = false;
    return true;
}

void EngineCore::Shutdown()
{
    if (!m_initialized)
    {
        return;
    }

    if (m_logger && m_timer && m_diagnostics)
    {
        m_logger->Info(
            std::string("Shutdown summary | loops = ") +
            std::to_string(m_diagnostics->GetLoopCount()) +
            " | uptime = " +
            std::to_string(m_diagnostics->GetUptimeSeconds()) +
            " | total time = " +
            std::to_string(m_timer->GetTotalTime())
        );
    }

    m_modules.ShutdownAll(m_context);

    m_logger = nullptr;
    m_timer = nullptr;
    m_diagnostics = nullptr;
    m_platform = nullptr;
    m_jobs = nullptr;
    m_memory = nullptr;

    m_initialized = false;
    m_running = false;
}

//
// ========================
// RUNTIME FLOW
// ========================
//

void EngineCore::BeginFrame()
{
    // future:
    // - frame markers
    // - profiling start
    // - timer frame start hook
}

void EngineCore::ServicePlatform()
{
    // placeholder for explicit platform polling later
    // currently platform update still happens through m_modules.UpdateAll(...)
}

void EngineCore::DispatchAsyncWork()
{
    // Core-owned work (still serial for now)
    m_modules.UpdateAll(m_context, 0.016f);

    // Placeholder:
    // future:
    // - Core schedules engine-owned async work
    // - App-driven jobs will come through Core APIs (SubmitJob)
}

void EngineCore::ServiceCoreWork()
{
    // placeholder:
    // work that must remain on the main/core thread
    // future:
    // - app-facing core callbacks
    // - main-thread-only engine coordination
    // - state publication
}

void EngineCore::ProcessEvents()
{
    ProcessPlatformEvents();
    HandleWindowStateChanges();
}

void EngineCore::ProcessCompletedWork()
{
    // placeholder:
    // future:
    // - collect completed job results
    // - integrate async outputs safely on the main thread
}

void EngineCore::ServiceScheduledWork()
{
    EmitScheduledDiagnostics();
}

void EngineCore::ServiceDeferredWork()
{
    // placeholder:
    // future:
    // - cleanup
    // - background maintenance
    // - async completion follow-up
}

void EngineCore::SynchronizeCriticalWork()
{
    // Placeholder:
    // future:
    // - wait only for engine-critical async work
    // - do not globally block unless a real dependency requires it
}

void EngineCore::EndFrame()
{
    // future:
    // - frame finalize
    // - present/swap
    // - frame stats
}

//
// ========================
// INTERNAL HELPERS
// ========================
//

void EngineCore::ProcessPlatformEvents()
{
    if (!m_platform)
    {
        return;
    }

    for (const auto& event : m_platform->GetEvents())
    {
        switch (event.type)
        {
        case PlatformWindow::Event::WindowActivated:
            if (m_logger)
            {
                m_logger->Info("Platform event: window activated.");
            }
            break;

        case PlatformWindow::Event::WindowDeactivated:
            if (m_logger)
            {
                m_logger->Warn("Platform event: window deactivated.");
            }
            break;

        case PlatformWindow::Event::WindowClosed:
            if (m_logger)
            {
                m_logger->Warn("Platform event: window closed.");
            }
            m_context.IsRunning = false;
            break;

        case PlatformWindow::Event::KeyDown:
            if (m_logger)
            {
                m_logger->Info(
                    std::string("Platform event: key down | code = ") +
                    std::to_string(event.a)
                );
            }

            if (event.a == 27)
            {
                if (m_logger)
                {
                    m_logger->Warn("Escape pressed. Requesting shutdown.");
                }

                m_context.IsRunning = false;
            }
            break;

        case PlatformWindow::Event::KeyUp:
            if (m_logger)
            {
                m_logger->Info(
                    std::string("Platform event: key up | code = ") +
                    std::to_string(event.a)
                );
            }
            break;

        default:
            break;
        }
    }

    m_platform->ClearEvents();
}

void EngineCore::HandleWindowStateChanges()
{
    if (!m_platform)
    {
        return;
    }

    if (m_platform->IsWindowOpen() != m_lastWindowOpen)
    {
        m_lastWindowOpen = m_platform->IsWindowOpen();

        if (m_logger)
        {
            m_logger->Info(
                std::string("Window open state changed: ") +
                (m_lastWindowOpen ? "true" : "false")
            );
        }
    }

    if (m_platform->IsWindowActive() != m_lastWindowActive)
    {
        m_lastWindowActive = m_platform->IsWindowActive();

        if (m_logger)
        {
            m_logger->Info(
                std::string("Window active state changed: ") +
                (m_lastWindowActive ? "true" : "false")
            );
        }
    }
}

void EngineCore::EmitScheduledDiagnostics()
{
    if (!m_timer || !m_diagnostics)
    {
        return;
    }

    const double totalTime = m_timer->GetTotalTime();

    if (totalTime >= m_nextDiagnosticsLogTime)
    {
        if (m_logger)
        {
            m_logger->Info(
                std::string("Emitting diagnostics report | time = ") +
                std::to_string(totalTime)
            );
        }

        m_diagnostics->EmitReport();
        m_nextDiagnosticsLogTime += 10.0;
    }
}

void EngineCore::RequestShutdown()
{
    m_context.IsRunning = false;
}

void EngineCore::LogInfo(const std::string& message)
{
    if (m_logger)
    {
        m_logger->Info(message);
    }
}

void EngineCore::LogWarn(const std::string& message)
{
    if (m_logger)
    {
        m_logger->Warn(message);
    }
}

void EngineCore::LogError(const std::string& message)
{
    if (m_logger)
    {
        m_logger->Error(message);
    }
}

bool EngineCore::SubmitJob(JobFunction job)
{
    if (!m_jobs)
    {
        return false;
    }

    m_jobs->Submit(std::move(job));
    return true;
}

void EngineCore::WaitForAllJobs()
{
    if (!m_jobs)
    {
        return;
    }

    m_jobs->WaitIdle();
}

unsigned int EngineCore::GetJobWorkerCount() const
{
    return m_jobs ? m_jobs->GetWorkerCount() : 0;
}

std::size_t EngineCore::GetQueuedJobCount() const
{
    return m_jobs ? m_jobs->GetQueuedJobCount() : 0;
}

unsigned int EngineCore::GetActiveJobCount() const
{
    return m_jobs ? m_jobs->GetActiveJobCount() : 0;
}

void* EngineCore::AllocateMemory(
    std::size_t size,
    MemoryClass memoryClass,
    std::size_t alignment,
    const char* owner)
{
    if (!m_memory)
    {
        return nullptr;
    }

    return m_memory->Allocate(size, memoryClass, alignment, owner);
}

void EngineCore::FreeMemory(void* ptr)
{
    if (!m_memory)
    {
        return;
    }

    m_memory->Free(ptr);
}

std::size_t EngineCore::GetCurrentMemoryBytes() const
{
    return m_memory ? m_memory->GetCurrentBytes() : 0;
}

std::size_t EngineCore::GetPeakMemoryBytes() const
{
    return m_memory ? m_memory->GetPeakBytes() : 0;
}

std::size_t EngineCore::GetMemoryAllocationCount() const
{
    return m_memory ? m_memory->GetAllocationCount() : 0;
}

std::size_t EngineCore::GetMemoryFreeCount() const
{
    return m_memory ? m_memory->GetFreeCount() : 0;
}

double EngineCore::GetTotalTime() const
{
    return m_timer ? m_timer->GetTotalTime() : 0.0;
}

double EngineCore::GetDiagnosticsUptime() const
{
    return m_diagnostics ? m_diagnostics->GetUptimeSeconds() : 0.0;
}

unsigned long long EngineCore::GetDiagnosticsLoopCount() const
{
    return m_diagnostics ? m_diagnostics->GetLoopCount() : 0;
}
