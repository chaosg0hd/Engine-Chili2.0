#pragma once

#include "engine_context.hpp"
#include "module_manager.hpp"
#include "../modules/gpu/gpu_compute_module.hpp"
#include "../modules/input/input_module.hpp"
#include "../modules/memory/memory_module.hpp"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <new>
#include <string>
#include <utility>
#include <vector>

class LoggerModule;
class TimerModule;
class DiagnosticsModule;
class PlatformModule;
class RenderModule;
class JobModule;
class FileModule;
class GpuComputeModule;

class EngineCore
{
public:
    using JobFunction = std::function<void()>;
    using FrameCallback = std::function<void(EngineCore&)>;

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
    void SetFrameCallback(FrameCallback callback);
    void ClearFrame(std::uint32_t color);
    void PutFramePixel(int x, int y, std::uint32_t color);
    void PresentFrame();
    int GetFrameWidth() const;
    int GetFrameHeight() const;

    bool FileExists(const std::string& path) const;
    bool DirectoryExists(const std::string& path) const;
    bool CreateDirectory(const std::string& path);
    bool WriteTextFile(const std::string& path, const std::string& content);
    bool ReadTextFile(const std::string& path, std::string& outContent) const;
    bool WriteBinaryFile(const std::string& path, const std::vector<std::byte>& content);
    bool ReadBinaryFile(const std::string& path, std::vector<std::byte>& outContent) const;
    bool DeleteFile(const std::string& path);
    std::uintmax_t GetFileSize(const std::string& path) const;
    std::string GetWorkingDirectory() const;

    bool IsGpuComputeAvailable() const;
    bool SubmitGpuTask(const GpuTaskDesc& task);
    void WaitForGpuIdle();
    std::string GetGpuBackendName() const;

    double GetDeltaTime() const;
    bool IsKeyDown(unsigned char key) const;
    bool WasKeyPressed(unsigned char key) const;
    bool WasKeyReleased(unsigned char key) const;
    bool IsMouseButtonDown(InputModule::MouseButton button) const;
    bool WasMouseButtonPressed(InputModule::MouseButton button) const;
    bool WasMouseButtonReleased(InputModule::MouseButton button) const;
    int GetMouseX() const;
    int GetMouseY() const;
    int GetMouseDeltaX() const;
    int GetMouseDeltaY() const;
    int GetMouseWheelDelta() const;

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
    RenderModule* m_render = nullptr;
    InputModule* m_input = nullptr;
    JobModule* m_jobs = nullptr;
    MemoryModule* m_memory = nullptr;
    FileModule* m_files = nullptr;
    GpuComputeModule* m_gpuCompute = nullptr;

    bool m_initialized = false;
    bool m_running = false;

    bool m_lastWindowOpen = false;
    bool m_lastWindowActive = false;

    FrameCallback m_frameCallback;
    std::wstring m_appOverlayText;
    double m_smoothedDeltaTime = 1.0 / 60.0;
    double m_smoothedFramesPerSecond = 60.0;
    double m_nextDiagnosticsLogTime = 10.0;
    double m_nextOverlayRefreshTime = 0.0;
    double m_targetFrameTime = 1.0 / 60.0;
};
