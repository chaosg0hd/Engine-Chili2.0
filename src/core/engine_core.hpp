#pragma once

#include "engine_context.hpp"
#include "module_manager.hpp"
#include "../modules/memory/memory_module.hpp"

#include <cstddef>
#include <functional>
#include <new>
#include <string>
#include <utility>

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
    void SetWindowOverlayText(const std::wstring& text);

    bool SubmitJob(JobFunction job);
    void WaitForAllJobs();

    unsigned int GetJobWorkerCount() const;
    std::size_t GetQueuedJobCount() const;
    unsigned int GetActiveJobCount() const;

    void* AllocateMemory(
        std::size_t size,
        MemoryClass memoryClass,
        std::size_t alignment = alignof(std::max_align_t),
        const char* owner = nullptr);

    template<typename T, typename... Args>
    T* NewObject(
        MemoryClass memoryClass,
        const char* owner,
        Args&&... args)
    {
        if (!m_memory)
        {
            return nullptr;
        }

        return m_memory->New<T>(memoryClass, owner, std::forward<Args>(args)...);
    }

    template<typename T>
    void DeleteObject(T* ptr)
    {
        if (!m_memory)
        {
            return;
        }

        m_memory->Delete(ptr);
    }

    template<typename T>
    T* NewArray(
        std::size_t count,
        MemoryClass memoryClass,
        const char* owner = nullptr)
    {
        if (!m_memory)
        {
            return nullptr;
        }

        return m_memory->NewArray<T>(count, memoryClass, owner);
    }

    template<typename T>
    void DeleteArray(T* ptr, std::size_t count)
    {
        if (!m_memory)
        {
            return;
        }

        m_memory->DeleteArray(ptr, count);
    }

    void FreeMemory(void* ptr);

    std::size_t GetCurrentMemoryBytes() const;
    std::size_t GetPeakMemoryBytes() const;
    std::size_t GetMemoryAllocationCount() const;
    std::size_t GetMemoryFreeCount() const;

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
    MemoryModule* m_memory = nullptr;

    bool m_initialized = false;
    bool m_running = false;

    bool m_lastWindowOpen = false;
    bool m_lastWindowActive = false;

    double m_nextDiagnosticsLogTime = 10.0;
};
