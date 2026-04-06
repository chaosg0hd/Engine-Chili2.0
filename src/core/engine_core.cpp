#include "engine_core.hpp"

#include "../modules/logger/logger_module.hpp"
#include "../modules/timer/timer_module.hpp"
#include "../modules/diagnostics/diagnostics_module.hpp"
#include "../modules/platform/platform_module.hpp"
#include "../modules/render/render_module.hpp"
#include "../modules/input/input_module.hpp"
#include "../modules/jobs/job_module.hpp"
#include "../modules/memory/memory_module.hpp"
#include "../modules/file/file_module.hpp"
#include "../modules/gpu/gpu_compute_module.hpp"

#include <windows.h>
#include <string>

#ifdef CreateDirectory
#undef CreateDirectory
#endif

#ifdef DeleteFile
#undef DeleteFile
#endif

#ifdef CreateDirectory
#undef CreateDirectory
#endif

#ifdef DeleteFile
#undef DeleteFile
#endif

EngineCore::EngineCore()
    : m_initialized(false),
      m_running(false),
      m_lastWindowOpen(false),
      m_lastWindowActive(false),
      m_smoothedDeltaTime(1.0 / 60.0),
      m_smoothedFramesPerSecond(60.0),
      m_nextDiagnosticsLogTime(10.0),
      m_nextOverlayRefreshTime(0.0),
      m_targetFrameTime(1.0 / 60.0)
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
    m_render = m_modules.AddModule<RenderModule>();
    m_input = m_modules.AddModule<InputModule>();
    m_jobs = m_modules.AddModule<JobModule>();
    m_memory = m_modules.AddModule<MemoryModule>();
    m_files = m_modules.AddModule<FileModule>();
    m_gpuCompute = m_modules.AddModule<GpuComputeModule>();

    if (m_render)
    {
        m_render->SetPlatformModule(m_platform);
    }

    if (m_input)
    {
        m_input->SetPlatformModule(m_platform);
    }

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
    m_smoothedDeltaTime = 1.0 / 60.0;
    m_smoothedFramesPerSecond = 60.0;
    m_nextDiagnosticsLogTime = 10.0;
    m_nextOverlayRefreshTime = 0.0;

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
    m_render = nullptr;
    m_input = nullptr;
    m_jobs = nullptr;
    m_memory = nullptr;
    m_files = nullptr;
    m_gpuCompute = nullptr;

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
    if (!m_timer)
    {
        return;
    }

    m_timer->Update(m_context, m_context.DeltaTime);

    float frameDelta = m_context.DeltaTime;
    if (frameDelta < 0.0f)
    {
        frameDelta = 0.0f;
    }
    else if (frameDelta > 0.050f)
    {
        frameDelta = 0.050f;
    }

    m_context.DeltaTime = frameDelta;
    m_smoothedDeltaTime = (m_smoothedDeltaTime * 0.90) + (static_cast<double>(frameDelta) * 0.10);
    m_smoothedFramesPerSecond = (m_smoothedDeltaTime > 0.0)
        ? (1.0 / m_smoothedDeltaTime)
        : 0.0;
}

void EngineCore::ServicePlatform()
{
    // placeholder for explicit platform polling later
    // currently platform update still happens through m_modules.UpdateAll(...)
}

void EngineCore::DispatchAsyncWork()
{
    const float frameDelta = m_context.DeltaTime;

    if (m_diagnostics)
    {
        m_diagnostics->Update(m_context, frameDelta);
    }

    if (m_platform)
    {
        m_platform->Update(m_context, frameDelta);
    }

    if (m_input)
    {
        m_input->Update(m_context, frameDelta);
    }

    if (m_render)
    {
        m_render->Update(m_context, frameDelta);
    }

    if (m_jobs)
    {
        m_jobs->Update(m_context, frameDelta);
    }

    if (m_memory)
    {
        m_memory->Update(m_context, frameDelta);
    }

    if (m_files)
    {
        m_files->Update(m_context, frameDelta);
    }

    if (m_gpuCompute)
    {
        m_gpuCompute->Update(m_context, frameDelta);
    }
}

void EngineCore::ServiceCoreWork()
{
    if (m_frameCallback)
    {
        m_frameCallback(*this);
    }
}

void EngineCore::ProcessEvents()
{
    ProcessPlatformEvents();
    HandleWindowStateChanges();

    if (WasKeyPressed(VK_ESCAPE))
    {
        if (m_logger)
        {
            m_logger->Warn("Escape pressed. Requesting shutdown.");
        }

        RequestShutdown();
    }
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
    if (m_platform && m_timer && m_diagnostics && m_context.TotalTime >= m_nextOverlayRefreshTime)
    {
        const std::wstring overlay =
            L"Project Engine\n"
            L"Uptime: " + std::to_wstring(m_timer->GetTotalTime()) + L"\n"
            L"Raw dt: " + std::to_wstring(m_context.DeltaTime) + L"\n"
            L"Smooth FPS: " + std::to_wstring(m_smoothedFramesPerSecond) + L"\n"
            L"Loops: " + std::to_wstring(m_diagnostics->GetLoopCount()) + L"\n"
            L"Window Active: " + std::wstring(m_platform->IsWindowActive() ? L"Yes" : L"No") + L"\n"
            L"Queued Jobs: " + std::to_wstring(GetQueuedJobCount()) + L"\n"
            L"Active Jobs: " + std::to_wstring(GetActiveJobCount()) + L"\n"
            L"Memory Current: " + std::to_wstring(GetCurrentMemoryBytes()) + L"\n"
            L"Memory Peak: " + std::to_wstring(GetPeakMemoryBytes()) + L"\n"
            L"Mouse: (" + std::to_wstring(GetMouseX()) + L", " + std::to_wstring(GetMouseY()) + L")\n"
            L"Mouse Delta: (" + std::to_wstring(GetMouseDeltaX()) + L", " + std::to_wstring(GetMouseDeltaY()) + L")\n"
            L"Wheel: " + std::to_wstring(GetMouseWheelDelta()) + L"\n"
            L"LMB: " + std::wstring(IsMouseButtonDown(InputModule::MouseButton::Left) ? L"Down" : L"Up") + L"\n"
            L"Esc Down: " + std::wstring(IsKeyDown(VK_ESCAPE) ? L"Yes" : L"No") + L"\n"
            L"Esc Pressed: " + std::wstring(WasKeyPressed(VK_ESCAPE) ? L"Yes" : L"No") + L"\n"
            L"Press ESC to exit" +
            (m_appOverlayText.empty() ? L"" : (L"\n\n" + m_appOverlayText));

        m_platform->SetOverlayText(overlay);
        m_nextOverlayRefreshTime = m_context.TotalTime + 0.10;
    }

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
    const double frameTime = static_cast<double>(m_context.DeltaTime);
    if (frameTime < m_targetFrameTime)
    {
        const double remainingTime = m_targetFrameTime - frameTime;
        const DWORD sleepMilliseconds = static_cast<DWORD>(remainingTime * 1000.0);
        if (sleepMilliseconds > 0)
        {
            Sleep(sleepMilliseconds);
        }
    }
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

void EngineCore::SetWindowOverlayText(const std::wstring& text)
{
    m_appOverlayText = text;
}

void EngineCore::SetFrameCallback(FrameCallback callback)
{
    m_frameCallback = std::move(callback);
}

void EngineCore::ClearFrame(std::uint32_t color)
{
    if (m_render)
    {
        m_render->Clear(color);
    }
}

void EngineCore::PutFramePixel(int x, int y, std::uint32_t color)
{
    if (m_render)
    {
        m_render->PutPixel(x, y, color);
    }
}

void EngineCore::PresentFrame()
{
    if (m_render)
    {
        m_render->Present();
    }
}

int EngineCore::GetFrameWidth() const
{
    return m_render ? m_render->GetBackbufferWidth() : 0;
}

int EngineCore::GetFrameHeight() const
{
    return m_render ? m_render->GetBackbufferHeight() : 0;
}

bool EngineCore::FileExists(const std::string& path) const
{
    return m_files ? m_files->FileExists(path) : false;
}

bool EngineCore::DirectoryExists(const std::string& path) const
{
    return m_files ? m_files->DirectoryExists(path) : false;
}

bool EngineCore::CreateDirectory(const std::string& path)
{
    return m_files ? m_files->CreateDirectory(path) : false;
}

bool EngineCore::WriteTextFile(const std::string& path, const std::string& content)
{
    return m_files ? m_files->WriteTextFile(path, content) : false;
}

bool EngineCore::ReadTextFile(const std::string& path, std::string& outContent) const
{
    if (!m_files)
    {
        outContent.clear();
        return false;
    }

    return m_files->ReadTextFile(path, outContent);
}

bool EngineCore::WriteBinaryFile(const std::string& path, const std::vector<std::byte>& content)
{
    return m_files ? m_files->WriteBinaryFile(path, content) : false;
}

bool EngineCore::ReadBinaryFile(const std::string& path, std::vector<std::byte>& outContent) const
{
    if (!m_files)
    {
        outContent.clear();
        return false;
    }

    return m_files->ReadBinaryFile(path, outContent);
}

bool EngineCore::DeleteFile(const std::string& path)
{
    return m_files ? m_files->DeleteFile(path) : false;
}

std::uintmax_t EngineCore::GetFileSize(const std::string& path) const
{
    return m_files ? m_files->GetFileSize(path) : 0;
}

std::string EngineCore::GetWorkingDirectory() const
{
    return m_files ? m_files->GetWorkingDirectory() : std::string();
}

bool EngineCore::IsGpuComputeAvailable() const
{
    return m_gpuCompute ? m_gpuCompute->IsGpuComputeAvailable() : false;
}

bool EngineCore::SubmitGpuTask(const GpuTaskDesc& task)
{
    return m_gpuCompute ? m_gpuCompute->SubmitGpuTask(task) : false;
}

void EngineCore::WaitForGpuIdle()
{
    if (m_gpuCompute)
    {
        m_gpuCompute->WaitForGpuIdle();
    }
}

std::string EngineCore::GetGpuBackendName() const
{
    return m_gpuCompute ? std::string(m_gpuCompute->GetBackendName()) : std::string("Unavailable");
}

double EngineCore::GetDeltaTime() const
{
    return static_cast<double>(m_context.DeltaTime);
}

bool EngineCore::IsKeyDown(unsigned char key) const
{
    return m_input ? m_input->IsKeyDown(key) : false;
}

bool EngineCore::WasKeyPressed(unsigned char key) const
{
    return m_input ? m_input->WasKeyPressed(key) : false;
}

bool EngineCore::WasKeyReleased(unsigned char key) const
{
    return m_input ? m_input->WasKeyReleased(key) : false;
}

bool EngineCore::IsMouseButtonDown(InputModule::MouseButton button) const
{
    return m_input ? m_input->IsMouseButtonDown(button) : false;
}

bool EngineCore::WasMouseButtonPressed(InputModule::MouseButton button) const
{
    return m_input ? m_input->WasMouseButtonPressed(button) : false;
}

bool EngineCore::WasMouseButtonReleased(InputModule::MouseButton button) const
{
    return m_input ? m_input->WasMouseButtonReleased(button) : false;
}

int EngineCore::GetMouseX() const
{
    return m_input ? m_input->GetMouseX() : 0;
}

int EngineCore::GetMouseY() const
{
    return m_input ? m_input->GetMouseY() : 0;
}

int EngineCore::GetMouseDeltaX() const
{
    return m_input ? m_input->GetMouseDeltaX() : 0;
}

int EngineCore::GetMouseDeltaY() const
{
    return m_input ? m_input->GetMouseDeltaY() : 0;
}

int EngineCore::GetMouseWheelDelta() const
{
    return m_input ? m_input->GetMouseWheelDelta() : 0;
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
