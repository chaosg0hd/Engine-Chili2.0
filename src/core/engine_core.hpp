#pragma once

#include "engine_context.hpp"
#include "module_manager.hpp"

#include <cstddef>
#include <functional>
#include <string>

class LoggerModule;
class TimerModule;
class DiagnosticsModule;
class PlatformModule;
class JobModule;

class EngineCore
{
public:
    using JobFunction = std::function<void()>;

public:
    EngineCore();

    bool Initialize();
    bool Run();
    void Shutdown();

    void RequestShutdown();

    void LogInfo(const std::string& message);
    void LogWarn(const std::string& message);
    void LogError(const std::string& message);

    bool SubmitJob(JobFunction job);
    void WaitForAllJobs();

    unsigned int GetJobWorkerCount() const;
    std::size_t GetQueuedJobCount() const;
    unsigned int GetActiveJobCount() const;

    double GetTotalTime() const;
    double GetDiagnosticsUptime() const;
    unsigned long long GetDiagnosticsLoopCount() const;

private:
    void BeginFrame();
    void ServicePlatform();
    void DispatchAsyncWork();
    void ServiceCoreWork();
    void ProcessEvents();
    void ProcessCompletedWork();
    void ServiceScheduledWork();
    void ServiceDeferredWork();
    void SynchronizeCriticalWork();
    void EndFrame();

    void ProcessPlatformEvents();
    void HandleWindowStateChanges();
    void EmitScheduledDiagnostics();

private:
    EngineContext m_context;
    ModuleManager m_modules;

    LoggerModule* m_logger = nullptr;
    TimerModule* m_timer = nullptr;
    DiagnosticsModule* m_diagnostics = nullptr;
    PlatformModule* m_platform = nullptr;
    JobModule* m_jobs = nullptr;

    bool m_initialized = false;
    bool m_running = false;

    bool m_lastWindowOpen = false;
    bool m_lastWindowActive = false;

    double m_nextDiagnosticsLogTime = 10.0;
};