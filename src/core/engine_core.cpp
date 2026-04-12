#include "engine_core.hpp"

#include "../modules/logger/logger_module.hpp"
#include "../modules/timer/timer_module.hpp"
#include "../modules/diagnostics/diagnostics_module.hpp"
#include "../modules/platform/platform_module.hpp"
#include "../modules/render/render_module.hpp"
#include "../modules/resources/resource_module.hpp"
#include "../modules/input/input_module.hpp"
#include "../modules/jobs/job_module.hpp"
#include "../modules/memory/memory_module.hpp"
#include "../modules/file/file_module.hpp"
#include "../modules/gpu/gpu_module.hpp"
#include "../modules/gpu/gpu_compute_module.hpp"
#include "../modules/prototypes/prototype_module.hpp"
#include "../modules/webview/webview_module.hpp"
#include "../modules/native_ui/native_ui_module.hpp"

#include <windows.h>

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <sstream>
#include <string>
#include <thread>

#ifdef CreateDirectory
#undef CreateDirectory
#endif

#ifdef DeleteFile
#undef DeleteFile
#endif

#ifdef CopyFile
#undef CopyFile
#endif

#ifdef MoveFile
#undef MoveFile
#endif

namespace
{
    std::string TrimString(const std::string& value)
    {
        const auto begin = std::find_if_not(
            value.begin(),
            value.end(),
            [](unsigned char character)
            {
                return std::isspace(character) != 0;
            });

        const auto end = std::find_if_not(
            value.rbegin(),
            value.rend(),
            [](unsigned char character)
            {
                return std::isspace(character) != 0;
            }).base();

        if (begin >= end)
        {
            return std::string();
        }

        return std::string(begin, end);
    }
}

EngineCore::EngineCore()
    : m_initialized(false),
      m_running(false),
      m_lastWindowOpen(false),
      m_lastWindowActive(false),
      m_overlayEnabled(true),
      m_overlayDirty(false),
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
    m_platformModule = m_modules.AddModule<PlatformModule>();
    m_platform = m_platformModule;
    m_gpuModule = m_modules.AddModule<GpuModule>();
    m_gpu = m_gpuModule;
    m_renderModule = m_modules.AddModule<RenderModule>();
    m_render = m_renderModule;
    m_resources = m_modules.AddModule<ResourceModule>();
    m_input = m_modules.AddModule<InputModule>();
    m_jobsModule = m_modules.AddModule<JobModule>();
    m_jobs = m_jobsModule;
    m_memory = m_modules.AddModule<MemoryModule>();
    m_filesModule = m_modules.AddModule<FileModule>();
    m_files = m_filesModule;
    m_prototypeModule = m_modules.AddModule<PrototypeModule>();
    m_prototypes = m_prototypeModule;
    m_gpuCompute = m_modules.AddModule<GpuComputeModule>();
    m_webViews = m_modules.AddModule<WebViewModule>();
    m_nativeUi = m_modules.AddModule<NativeUiModule>();

    m_context.Logger = m_logger;
    m_context.Platform = m_platform;
    m_context.Gpu = m_gpu;
    m_context.Render = m_render;
    m_context.Resources = m_resources;
    m_context.Jobs = m_jobs;
    m_context.Files = m_files;
    m_context.Memory = m_memory;
    m_context.Prototypes = m_prototypes;

    if (!m_modules.InitializeAll(m_context))
    {
        if (m_logger)
        {
            m_logger->Error("EngineCore initialization failed.");
        }

        m_modules.ShutdownAll(m_context);
        m_modules.Clear();
        ResetModuleReferences();
        m_context.IsRunning = false;
        return false;
    }

    m_modules.StartupAll(m_context);

    m_lastWindowOpen = (m_platform != nullptr) ? m_platform->IsWindowOpen() : false;
    m_lastWindowActive = (m_platform != nullptr) ? m_platform->IsWindowActive() : false;
    m_smoothedDeltaTime = 1.0 / 60.0;
    m_smoothedFramesPerSecond = 60.0;
    m_nextDiagnosticsLogTime = 10.0;
    m_nextOverlayRefreshTime = 0.0;
    m_overlayDirty = true;
    m_lastFrameDuration = 0.0;
    m_lastFrameWorkDuration = 0.0;
    m_lastFrameWaitDuration = 0.0;
    m_lastFrameLateness = 0.0;
    m_maxFrameLateness = 0.0;
    m_lateFrameCount = 0;
    m_lastFrameSleepCount = 0;
    m_lastFrameYieldCount = 0;
    m_adaptiveSleepMargin = 0.0010;
    m_adaptiveYieldMargin = 0.0002;
    m_isBehindSchedule = false;
    ResetSettingsToDefaults();

    if (!LoadSettings())
    {
        if (m_logger)
        {
            m_logger->Warn(
                std::string("EngineCore settings load failed. Using defaults from ") +
                m_settingsPath
            );
        }
    }

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
        m_frameStartTime = FrameClock::now();

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

bool EngineCore::Tick()
{
    if (!m_initialized)
    {
        return false;
    }

    if (!m_running)
    {
        m_running = true;
        m_context.IsRunning = true;
    }

    if (!m_context.IsRunning)
    {
        m_running = false;
        return false;
    }

    m_frameStartTime = FrameClock::now();

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

    if (!m_context.IsRunning)
    {
        m_running = false;
    }

    return m_context.IsRunning;
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
    m_modules.Clear();
    ResetModuleReferences();
    m_context.IsRunning = false;
    m_context.DeltaTime = 0.0f;
    m_context.TotalTime = 0.0;

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

    if (m_platformModule)
    {
        m_platformModule->Update(m_context, frameDelta);
    }

    if (m_gpuModule)
    {
        m_gpuModule->Update(m_context, frameDelta);
    }

    if (m_input)
    {
        m_input->Update(m_context, frameDelta);
    }

    if (m_jobsModule)
    {
        m_jobsModule->Update(m_context, frameDelta);
    }

    if (m_memory)
    {
        m_memory->Update(m_context, frameDelta);
    }

    if (m_prototypeModule)
    {
        m_prototypeModule->Update(m_context, frameDelta);
    }

    if (m_filesModule)
    {
        m_filesModule->Update(m_context, frameDelta);
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

    if (m_renderModule)
    {
        m_renderModule->Update(m_context, m_context.DeltaTime);
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
    if (m_platform && m_overlayEnabled && (m_overlayDirty || m_context.TotalTime >= m_nextOverlayRefreshTime))
    {
        m_platform->SetOverlayText(m_appOverlayText);
        m_overlayDirty = false;
        m_nextOverlayRefreshTime = m_context.TotalTime + 0.50;
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
    const double workDuration = std::chrono::duration<double>(
        FrameClock::now() - m_frameStartTime
    ).count();
    m_lastFrameWorkDuration = workDuration;
    m_lastFrameWaitDuration = 0.0;
    m_lastFrameSleepCount = 0;
    m_lastFrameYieldCount = 0;

    if (m_targetFrameTime > 0.0)
    {
        const FrameClock::duration targetFrameDuration =
            std::chrono::duration_cast<FrameClock::duration>(std::chrono::duration<double>(m_targetFrameTime));
        const FrameClock::time_point targetFrameEnd = m_frameStartTime + targetFrameDuration;
        const FrameClock::time_point waitStartTime = FrameClock::now();
        const FrameClock::duration sleepMarginDuration =
            std::chrono::duration_cast<FrameClock::duration>(std::chrono::duration<double>(m_adaptiveSleepMargin));
        const FrameClock::duration yieldMarginDuration =
            std::chrono::duration_cast<FrameClock::duration>(std::chrono::duration<double>(m_adaptiveYieldMargin));

        // Sleep away the coarse remainder first, then yield and finally spin through the last tiny slice.
        while (FrameClock::now() + sleepMarginDuration < targetFrameEnd)
        {
            Sleep(1);
            ++m_lastFrameSleepCount;
        }

        while (FrameClock::now() + yieldMarginDuration < targetFrameEnd)
        {
            std::this_thread::yield();
            ++m_lastFrameYieldCount;
        }

        while (FrameClock::now() < targetFrameEnd)
        {
        }

        m_lastFrameWaitDuration = std::chrono::duration<double>(
            FrameClock::now() - waitStartTime
        ).count();
    }

    const double measuredFrameDuration = std::chrono::duration<double>(
        FrameClock::now() - m_frameStartTime
    ).count();

    UpdateFramePacing(measuredFrameDuration);
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

void EngineCore::ResetModuleReferences()
{
    m_logger = nullptr;
    m_timer = nullptr;
    m_diagnostics = nullptr;
    m_platformModule = nullptr;
    m_platform = nullptr;
    m_renderModule = nullptr;
    m_render = nullptr;
    m_input = nullptr;
    m_jobsModule = nullptr;
    m_jobs = nullptr;
    m_memory = nullptr;
    m_filesModule = nullptr;
    m_files = nullptr;
    m_prototypeModule = nullptr;
    m_prototypes = nullptr;
    m_resources = nullptr;
    m_gpuModule = nullptr;
    m_gpu = nullptr;
    m_gpuCompute = nullptr;
    m_webViews = nullptr;
    m_nativeUi = nullptr;
    m_context.Logger = nullptr;
    m_context.Platform = nullptr;
    m_context.Gpu = nullptr;
    m_context.Render = nullptr;
    m_context.Resources = nullptr;
    m_context.Jobs = nullptr;
    m_context.Files = nullptr;
    m_context.Memory = nullptr;
    m_context.Prototypes = nullptr;
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

std::string EngineCore::GetLogFilePath() const
{
    return m_logger ? m_logger->GetLogFilePath() : std::string();
}

bool EngineCore::IsFileLoggingAvailable() const
{
    return m_logger ? m_logger->IsFileLoggingAvailable() : false;
}

std::wstring EngineCore::BuildDebugViewText() const
{
    const double deltaTime = GetDeltaTime();
    const double framesPerSecond = (deltaTime > 0.0) ? (1.0 / deltaTime) : 0.0;
    const unsigned int workerCount = GetJobWorkerCount();
    const unsigned int idleWorkerCount = GetIdleJobWorkerCount();
    const unsigned int busyWorkerCount = (idleWorkerCount >= workerCount) ? 0U : (workerCount - idleWorkerCount);
    const double workerUtilization = (workerCount > 0U)
        ? (static_cast<double>(busyWorkerCount) / static_cast<double>(workerCount)) * 100.0
        : 0.0;
    const std::wstring leftMouseState = IsMouseButtonDown(InputModule::MouseButton::Left) ? L"Down" : L"Up";
    const std::wstring rightMouseState = IsMouseButtonDown(InputModule::MouseButton::Right) ? L"Down" : L"Up";
    const std::wstring middleMouseState = IsMouseButtonDown(InputModule::MouseButton::Middle) ? L"Down" : L"Up";

    return
        L"DEBUG VIEW\n"
        L"Runtime\n"
        L"  Uptime: " + std::to_wstring(GetTotalTime()) + L"s\n"
        L"  Delta Time: " + std::to_wstring(deltaTime) + L"s\n"
        L"  FPS: " + std::to_wstring(framesPerSecond) + L"\n"
        L"  FPS Limit: " + std::to_wstring(GetFpsLimit()) + L"\n"
        L"  Loops: " + std::to_wstring(GetDiagnosticsLoopCount()) + L"\n"
        L"\n"
        L"Frame Pacing\n"
        L"  Target FPS: " + std::to_wstring(GetTargetFramesPerSecond()) + L"\n"
        L"  Target Frame Time: " + std::to_wstring(GetTargetFrameTime()) + L"s\n"
        L"  Last Frame Duration: " + std::to_wstring(GetLastFrameDuration()) + L"s\n"
        L"  Last Frame Work: " + std::to_wstring(GetLastFrameWorkDuration()) + L"s\n"
        L"  Last Frame Wait: " + std::to_wstring(GetLastFrameWaitDuration()) + L"s\n"
        L"  Last Frame Lateness: " + std::to_wstring(GetLastFrameLateness()) + L"s\n"
        L"  Max Frame Lateness: " + std::to_wstring(GetMaxFrameLateness()) + L"s\n"
        L"  Late Frame Count: " + std::to_wstring(GetLateFrameCount()) + L"\n"
        L"  Sleep/Yield Count: " + std::to_wstring(GetLastFrameSleepCount()) + L" / " + std::to_wstring(GetLastFrameYieldCount()) + L"\n"
        L"  Adaptive Sleep Margin: " + std::to_wstring(GetAdaptiveSleepMargin()) + L"s\n"
        L"  Adaptive Yield Margin: " + std::to_wstring(GetAdaptiveYieldMargin()) + L"s\n"
        L"  Behind Schedule: " + std::wstring(IsBehindSchedule() ? L"Yes" : L"No") + L"\n"
        L"\n"
        L"Render\n"
        L"  Frame: " + std::to_wstring(GetFrameWidth()) + L" x " + std::to_wstring(GetFrameHeight()) + L"\n"
        L"  Aspect Ratio: " + std::to_wstring(GetFrameAspectRatio()) + L"\n"
        L"\n"
        L"Input\n"
        L"  Mouse: (" + std::to_wstring(GetMouseX()) + L", " + std::to_wstring(GetMouseY()) + L")\n"
        L"  Mouse Normalized: (" + std::to_wstring(GetMouseNormalizedX()) + L", " + std::to_wstring(GetMouseNormalizedY()) + L")\n"
        L"  Mouse Delta: (" + std::to_wstring(GetMouseDeltaX()) + L", " + std::to_wstring(GetMouseDeltaY()) + L")\n"
        L"  Wheel: " + std::to_wstring(GetMouseWheelDelta()) + L"\n"
        L"  LMB/RMB/MMB: " + leftMouseState + L" / " + rightMouseState + L" / " + middleMouseState + L"\n"
        L"  Any Key Pressed: " + std::wstring(IsAnyKeyPressed() ? L"Yes" : L"No") + L"\n"
        L"\n"
        L"Jobs\n"
        L"  Workers: " + std::to_wstring(workerCount) + L"\n"
        L"  Consumer Workers: " + std::to_wstring(busyWorkerCount) + L" / " + std::to_wstring(workerCount) + L" busy\n"
        L"  Worker Utilization: " + std::to_wstring(workerUtilization) + L"%\n"
        L"  Idle Workers: " + std::to_wstring(idleWorkerCount) + L"\n"
        L"  Queued: " + std::to_wstring(GetQueuedJobCount()) + L"\n"
        L"  Peak Queued: " + std::to_wstring(GetPeakQueuedJobCount()) + L"\n"
        L"  Active: " + std::to_wstring(GetActiveJobCount()) + L"\n"
        L"  Failed: " + std::to_wstring(GetFailedJobCount()) + L"\n"
        L"\n"
        L"Resources\n"
        L"  Total: " + std::to_wstring(GetTrackedResourceCount()) + L"\n"
        L"  Queued/Loading: " + std::to_wstring(GetResourceCountByState(ResourceState::Queued)) + L" / " + std::to_wstring(GetResourceCountByState(ResourceState::Loading)) + L"\n"
        L"  Processing/Uploading: " + std::to_wstring(GetResourceCountByState(ResourceState::Processing)) + L" / " + std::to_wstring(GetResourceCountByState(ResourceState::Uploading)) + L"\n"
        L"  Ready/Failed: " + std::to_wstring(GetResourceCountByState(ResourceState::Ready)) + L" / " + std::to_wstring(GetResourceCountByState(ResourceState::Failed)) + L"\n"
        L"\n"
        L"Renderer\n"
        L"  Scene Items: " + std::to_wstring(GetRenderSubmittedItemCount()) + L"\n"
        L"  Legacy Commands: " + std::to_wstring(GetRenderLegacyCompatibilityCommandCount()) + L"\n"
        L"\n"
        L"Memory\n"
        L"  Current Bytes: " + std::to_wstring(GetCurrentMemoryBytes()) + L"\n"
        L"  Peak Bytes: " + std::to_wstring(GetPeakMemoryBytes()) + L"\n"
        L"  Frame Bytes: " + std::to_wstring(GetMemoryBytesByClass(MemoryClass::Frame)) + L" (peak " + std::to_wstring(GetPeakMemoryBytesByClass(MemoryClass::Frame)) + L")\n"
        L"  Upload Bytes: " + std::to_wstring(GetMemoryBytesByClass(MemoryClass::Upload)) + L" (peak " + std::to_wstring(GetPeakMemoryBytesByClass(MemoryClass::Upload)) + L")\n"
        L"  Resource Bytes: " + std::to_wstring(GetMemoryBytesByClass(MemoryClass::Resource)) + L" (peak " + std::to_wstring(GetPeakMemoryBytesByClass(MemoryClass::Resource)) + L")\n"
        L"  Temporary Bytes: " + std::to_wstring(GetMemoryBytesByClass(MemoryClass::Temporary)) + L" (peak " + std::to_wstring(GetPeakMemoryBytesByClass(MemoryClass::Temporary)) + L")\n"
        L"  Debug Bytes: " + std::to_wstring(GetMemoryBytesByClass(MemoryClass::Debug)) + L" (peak " + std::to_wstring(GetPeakMemoryBytesByClass(MemoryClass::Debug)) + L")\n"
        L"  Allocations: " + std::to_wstring(GetMemoryAllocationCount()) + L"\n"
        L"  Frees: " + std::to_wstring(GetMemoryFreeCount()) + L"\n"
        L"\n"
        L"System\n"
        L"  Settings: " + ToWideString(GetSettingsPath()) + L"\n"
        L"  Settings Abs: " + ToWideString(GetAbsolutePath(GetSettingsPath())) + L"\n"
        L"  Working Dir: " + ToWideString(GetWorkingDirectory()) + L"\n"
        L"  Runtime Log: " + ToWideString(GetLogFilePath()) + L"\n"
        L"  Window Open/Active: " + std::wstring(IsWindowOpen() ? L"Yes" : L"No") + L" / " + std::wstring(IsWindowActive() ? L"Yes" : L"No") + L"\n"
        L"  Window Size: " + std::to_wstring(GetWindowWidth()) + L" x " + std::to_wstring(GetWindowHeight()) + L"\n"
        L"  GPU Backend: " + ToWideString(GetGpuBackendName()) + L"\n"
        L"  GPU Available: " + std::wstring(IsGpuComputeAvailable() ? L"Yes" : L"No") + L"\n"
        L"  GPU Tracked Resources: " + std::to_wstring(GetGpuTrackedResourceCount()) + L"\n"
        L"  GPU Tracked Bytes: " + std::to_wstring(GetGpuTrackedResourceBytes()) + L"\n"
        L"\n"
        L"Controls\n"
        L"  Tab = Toggle Renderer\n"
        L"  Escape = Exit";
}

void EngineCore::ShowDebugView()
{
    SetWindowOverlayText(BuildDebugViewText());
}

void EngineCore::ClearWindowOverlayText()
{
    SetWindowOverlayText(L"");
}

bool EngineCore::IsWindowOverlayEnabled() const
{
    return m_overlayEnabled;
}

void EngineCore::SetWindowOverlayEnabled(bool enabled)
{
    m_overlayEnabled = enabled;
    m_overlayDirty = true;

    if (m_platform)
    {
        m_platform->SetOverlayText(enabled ? m_appOverlayText : std::wstring());
        m_overlayDirty = false;
        m_nextOverlayRefreshTime = m_context.TotalTime + 0.50;
    }
}

void EngineCore::AppendWindowOverlayText(const std::wstring& text)
{
    if (text.empty())
    {
        return;
    }

    if (!m_appOverlayText.empty())
    {
        m_appOverlayText += text;
    }
    else
    {
        m_appOverlayText = text;
    }

    m_overlayDirty = true;
}

void EngineCore::SetWindowOverlayText(const std::wstring& text)
{
    if (m_appOverlayText == text)
    {
        return;
    }

    m_appOverlayText = text;
    m_overlayDirty = true;
}

void EngineCore::SetFrameCallback(FrameCallback callback)
{
    m_frameCallback = std::move(callback);
}

bool EngineCore::LoadSettings()
{
    return LoadSettings(m_settingsPath);
}

bool EngineCore::LoadSettings(const std::string& path)
{
    m_settingsPath = path.empty() ? std::string("config/engine.ini") : path;
    ResetSettingsToDefaults();

    if (!m_files)
    {
        return false;
    }

    std::string settingsText;
    if (!m_files->ReadTextFile(m_settingsPath, settingsText))
    {
        return SaveSettings(m_settingsPath);
    }

    if (!ApplySettingsFromText(settingsText))
    {
        return false;
    }

    return SaveSettings(m_settingsPath);
}

bool EngineCore::SaveSettings() const
{
    return SaveSettings(m_settingsPath);
}

bool EngineCore::SaveSettings(const std::string& path) const
{
    if (!m_files)
    {
        return false;
    }

    const std::string settingsPath = path.empty() ? m_settingsPath : path;
    return m_files->WriteTextFile(settingsPath, BuildSettingsText());
}

void EngineCore::ResetSettingsToDefaults()
{
    ApplyTargetFramesPerSecond(60.0);
}

const std::string& EngineCore::GetSettingsPath() const
{
    return m_settingsPath;
}

double EngineCore::GetFpsLimit() const
{
    return m_fpsLimit;
}

void EngineCore::SetFpsLimit(double framesPerSecond)
{
    SetTargetFramesPerSecond(framesPerSecond);
}

void EngineCore::SetTargetFramesPerSecond(double framesPerSecond)
{
    if (framesPerSecond <= 0.0)
    {
        return;
    }

    ApplyTargetFramesPerSecond(framesPerSecond);
}

double EngineCore::GetTargetFramesPerSecond() const
{
    return m_targetFramesPerSecond;
}

double EngineCore::GetTargetFrameTime() const
{
    return m_targetFrameTime;
}

double EngineCore::GetLastFrameDuration() const
{
    return m_lastFrameDuration;
}

double EngineCore::GetLastFrameWorkDuration() const
{
    return m_lastFrameWorkDuration;
}

double EngineCore::GetLastFrameWaitDuration() const
{
    return m_lastFrameWaitDuration;
}

double EngineCore::GetLastFrameLateness() const
{
    return m_lastFrameLateness;
}

double EngineCore::GetMaxFrameLateness() const
{
    return m_maxFrameLateness;
}

unsigned long long EngineCore::GetLateFrameCount() const
{
    return m_lateFrameCount;
}

unsigned long long EngineCore::GetLastFrameSleepCount() const
{
    return m_lastFrameSleepCount;
}

unsigned long long EngineCore::GetLastFrameYieldCount() const
{
    return m_lastFrameYieldCount;
}

double EngineCore::GetAdaptiveSleepMargin() const
{
    return m_adaptiveSleepMargin;
}

double EngineCore::GetAdaptiveYieldMargin() const
{
    return m_adaptiveYieldMargin;
}

bool EngineCore::IsBehindSchedule() const
{
    return m_isBehindSchedule;
}

void EngineCore::SubmitRenderFrame(const FramePrototype& frame)
{
    if (m_render)
    {
        m_render->SubmitFrame(frame);
    }
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

void EngineCore::DrawFrameLine(int x0, int y0, int x1, int y1, std::uint32_t color)
{
    if (m_render)
    {
        m_render->DrawLine(x0, y0, x1, y1, color);
    }
}

void EngineCore::DrawFrameRect(int x, int y, int width, int height, std::uint32_t color)
{
    if (m_render)
    {
        m_render->DrawRect(x, y, width, height, color);
    }
}

void EngineCore::FillFrameRect(int x, int y, int width, int height, std::uint32_t color)
{
    if (m_render)
    {
        m_render->FillRect(x, y, width, height, color);
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

double EngineCore::GetFrameAspectRatio() const
{
    return m_render ? m_render->GetAspectRatio() : 0.0;
}

std::size_t EngineCore::GetRenderSubmittedItemCount() const
{
    return m_render ? m_render->GetSubmittedItemCount() : 0U;
}

std::size_t EngineCore::GetRenderLegacyCompatibilityCommandCount() const
{
    return m_render ? m_render->GetLegacyCompatibilityCommandCount() : 0U;
}

void EngineCore::DrawFrameGrid(int cellSize, std::uint32_t color)
{
    if (m_render)
    {
        m_render->DrawGrid(cellSize, color);
    }
}

void EngineCore::DrawFrameCrosshair(int x, int y, int size, std::uint32_t color)
{
    if (m_render)
    {
        m_render->DrawCrosshair(x, y, size, color);
    }
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

bool EngineCore::WriteJsonFile(const std::string& path, const std::string& jsonContent)
{
    return m_files ? m_files->WriteJsonFile(path, jsonContent) : false;
}

bool EngineCore::ReadJsonFile(const std::string& path, std::string& outJsonContent) const
{
    if (!m_files)
    {
        outJsonContent.clear();
        return false;
    }

    return m_files->ReadJsonFile(path, outJsonContent);
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

std::vector<std::string> EngineCore::ListDirectory(const std::string& path) const
{
    return m_files ? m_files->ListDirectory(path) : std::vector<std::string>();
}

std::vector<std::string> EngineCore::ListFiles(const std::string& path) const
{
    return m_files ? m_files->ListFiles(path) : std::vector<std::string>();
}

std::vector<std::string> EngineCore::ListDirectories(const std::string& path) const
{
    return m_files ? m_files->ListDirectories(path) : std::vector<std::string>();
}

std::string EngineCore::GetAbsolutePath(const std::string& path) const
{
    return m_files ? m_files->GetAbsolutePath(path) : std::string();
}

std::string EngineCore::NormalizePath(const std::string& path) const
{
    return m_files ? m_files->NormalizePath(path) : std::string();
}

bool EngineCore::CopyFile(const std::string& source, const std::string& destination)
{
    return m_files ? m_files->CopyFile(source, destination) : false;
}

bool EngineCore::MoveFile(const std::string& source, const std::string& destination)
{
    return m_files ? m_files->MoveFile(source, destination) : false;
}

ResourceHandle EngineCore::RequestResource(const std::string& assetId, ResourceKind kind)
{
    return m_resources ? m_resources->RequestResource(assetId, kind) : 0U;
}

bool EngineCore::UnloadResource(ResourceHandle handle)
{
    return m_resources ? m_resources->UnloadResource(handle) : false;
}

ResourceState EngineCore::GetResourceState(ResourceHandle handle) const
{
    return m_resources ? m_resources->GetResourceState(handle) : ResourceState::Unloaded;
}

ResourceKind EngineCore::GetResourceKind(ResourceHandle handle) const
{
    return m_resources ? m_resources->GetResourceKind(handle) : ResourceKind::Unknown;
}

std::string EngineCore::GetResourceAssetId(ResourceHandle handle) const
{
    return m_resources ? m_resources->GetAssetId(handle) : std::string();
}

std::string EngineCore::GetResourceResolvedPath(ResourceHandle handle) const
{
    return m_resources ? m_resources->GetResolvedPath(handle) : std::string();
}

std::string EngineCore::GetResourceLastError(ResourceHandle handle) const
{
    return m_resources ? m_resources->GetLastError(handle) : std::string();
}

std::size_t EngineCore::GetResourceSourceByteSize(ResourceHandle handle) const
{
    return m_resources ? m_resources->GetSourceByteSize(handle) : 0U;
}

std::string EngineCore::GetResourceSourceText(ResourceHandle handle) const
{
    return m_resources ? m_resources->GetSourceText(handle) : std::string();
}

GpuResourceHandle EngineCore::GetResourceGpuHandle(ResourceHandle handle) const
{
    return m_resources ? m_resources->GetGpuResourceHandle(handle) : 0U;
}

std::size_t EngineCore::GetResourceUploadedByteSize(ResourceHandle handle) const
{
    return m_resources ? m_resources->GetUploadedByteSize(handle) : 0U;
}

bool EngineCore::IsResourceReady(ResourceHandle handle) const
{
    return m_resources ? m_resources->IsResourceReady(handle) : false;
}

std::size_t EngineCore::GetResourceCountByState(ResourceState state) const
{
    return m_resources ? m_resources->GetResourceCountByState(state) : 0U;
}

std::size_t EngineCore::GetTrackedResourceCount() const
{
    return m_resources ? m_resources->GetResourceCount() : 0;
}

bool EngineCore::RegisterPrototype(std::unique_ptr<IPrototype> prototype)
{
    return m_prototypes ? m_prototypes->RegisterPrototype(std::move(prototype)) : false;
}

PrototypeId EngineCore::LoadPrototypeAsset(const std::string& assetId)
{
    return m_prototypes ? m_prototypes->LoadPrototypeAsset(assetId) : 0U;
}

bool EngineCore::UnregisterPrototype(PrototypeId prototypeId)
{
    return m_prototypes ? m_prototypes->UnregisterPrototype(prototypeId) : false;
}

const IPrototype* EngineCore::GetPrototype(PrototypeId prototypeId) const
{
    return m_prototypes ? m_prototypes->GetPrototype(prototypeId) : nullptr;
}

bool EngineCore::HasPrototype(PrototypeId prototypeId) const
{
    return m_prototypes ? m_prototypes->HasPrototype(prototypeId) : false;
}

PrototypeInstanceHandle EngineCore::CreatePrototypeInstance(PrototypeId prototypeId)
{
    return m_prototypes ? m_prototypes->CreateInstance(prototypeId) : 0U;
}

bool EngineCore::DestroyPrototypeInstance(PrototypeInstanceHandle instanceHandle)
{
    return m_prototypes ? m_prototypes->DestroyInstance(instanceHandle) : false;
}

void* EngineCore::GetPrototypeInstanceRuntimeData(PrototypeInstanceHandle instanceHandle) const
{
    return m_prototypes ? m_prototypes->GetInstanceRuntimeData(instanceHandle) : nullptr;
}

PrototypeId EngineCore::GetPrototypeInstancePrototypeId(PrototypeInstanceHandle instanceHandle) const
{
    return m_prototypes ? m_prototypes->GetInstancePrototypeId(instanceHandle) : 0U;
}

std::size_t EngineCore::GetRegisteredPrototypeCount() const
{
    return m_prototypes ? m_prototypes->GetPrototypeCount() : 0U;
}

std::size_t EngineCore::GetLivePrototypeInstanceCount() const
{
    return m_prototypes ? m_prototypes->GetLiveInstanceCount() : 0U;
}

bool EngineCore::IsGpuComputeAvailable() const
{
    if (m_gpuModule && m_gpuModule->SupportsComputeDispatch())
    {
        return true;
    }

    return m_gpuCompute ? m_gpuCompute->IsGpuComputeAvailable() : false;
}

bool EngineCore::SubmitGpuTask(const GpuTaskDesc& task)
{
    if (m_gpuModule && m_gpuModule->SubmitGpuTask(task))
    {
        return true;
    }

    return m_gpuCompute ? m_gpuCompute->SubmitGpuTask(task) : false;
}

void EngineCore::WaitForGpuIdle()
{
    if (m_gpuModule)
    {
        m_gpuModule->WaitForGpuIdle();
    }

    if (m_gpuCompute)
    {
        m_gpuCompute->WaitForGpuIdle();
    }
}

std::string EngineCore::GetGpuBackendName() const
{
    if (m_gpu)
    {
        return std::string(m_gpu->GetBackendName());
    }

    return m_gpuCompute ? std::string(m_gpuCompute->GetBackendName()) : std::string("Unavailable");
}

bool EngineCore::SupportsGpuBuffers() const
{
    if (m_gpuModule && m_gpuModule->SupportsGpuBuffers())
    {
        return true;
    }

    return m_gpuCompute ? m_gpuCompute->SupportsGpuBuffers() : false;
}

bool EngineCore::SupportsComputeDispatch() const
{
    if (m_gpuModule && m_gpuModule->SupportsComputeDispatch())
    {
        return true;
    }

    return m_gpuCompute ? m_gpuCompute->SupportsComputeDispatch() : false;
}

std::string EngineCore::GetGpuCapabilitySummary() const
{
    if (m_gpuModule && m_gpuModule->SupportsComputeDispatch())
    {
        return std::string("backend=") + m_gpuModule->GetBackendName() + " | available=true | buffers=true | dispatch=true";
    }

    return m_gpuCompute ? m_gpuCompute->GetCapabilitySummary() : std::string("backend=Unavailable | available=false | buffers=false | dispatch=false");
}

std::size_t EngineCore::GetGpuTrackedResourceCount() const
{
    return m_gpu ? m_gpu->GetResourceCount() : 0U;
}

std::size_t EngineCore::GetGpuTrackedResourceBytes() const
{
    return m_gpu ? m_gpu->GetTotalResourceBytes() : 0U;
}

EngineCore::WebDialogHandle EngineCore::CreateWebDialog(const WebDialogDesc& desc)
{
    return m_webViews ? m_webViews->CreateWebDialogInstance(desc) : 0U;
}

bool EngineCore::DestroyWebDialog(WebDialogHandle handle)
{
    return m_webViews ? m_webViews->DestroyWebDialogInstance(handle) : false;
}

void EngineCore::DestroyAllWebDialogs()
{
    if (m_webViews)
    {
        m_webViews->DestroyAllDialogs();
    }
}

bool EngineCore::SetWebDialogContentPath(WebDialogHandle handle, const std::string& contentPath)
{
    return m_webViews ? m_webViews->SetDialogContentPath(handle, contentPath) : false;
}

bool EngineCore::SetWebDialogDockMode(WebDialogHandle handle, WebDialogDockMode dockMode, int dockSize)
{
    return m_webViews ? m_webViews->SetDialogDockMode(handle, dockMode, dockSize) : false;
}

bool EngineCore::SetWebDialogBounds(WebDialogHandle handle, const WebDialogRect& rect)
{
    return m_webViews ? m_webViews->SetDialogBounds(handle, rect) : false;
}

bool EngineCore::SetWebDialogVisible(WebDialogHandle handle, bool visible)
{
    return m_webViews ? m_webViews->SetDialogVisible(handle, visible) : false;
}

bool EngineCore::IsWebDialogReady(WebDialogHandle handle) const
{
    return m_webViews ? m_webViews->IsDialogReady(handle) : false;
}

bool EngineCore::IsWebDialogOpen(WebDialogHandle handle) const
{
    return m_webViews ? m_webViews->IsDialogOpen(handle) : false;
}

WebDialogRect EngineCore::GetWebDialogBounds(WebDialogHandle handle) const
{
    return m_webViews ? m_webViews->GetDialogBounds(handle) : WebDialogRect{};
}

EngineCore::NativeButtonHandle EngineCore::CreateNativeButton(const NativeButtonDesc& desc)
{
    return m_nativeUi ? m_nativeUi->CreateButton(desc) : 0U;
}

bool EngineCore::DestroyNativeButton(NativeButtonHandle handle)
{
    return m_nativeUi ? m_nativeUi->DestroyButton(handle) : false;
}

void EngineCore::DestroyAllNativeButtons()
{
    if (m_nativeUi)
    {
        m_nativeUi->DestroyAllButtons();
    }
}

bool EngineCore::SetNativeButtonBounds(NativeButtonHandle handle, const NativeControlRect& rect)
{
    return m_nativeUi ? m_nativeUi->SetButtonBounds(handle, rect) : false;
}

bool EngineCore::SetNativeButtonText(NativeButtonHandle handle, const std::wstring& text)
{
    return m_nativeUi ? m_nativeUi->SetButtonText(handle, text) : false;
}

bool EngineCore::SetNativeButtonVisible(NativeButtonHandle handle, bool visible)
{
    return m_nativeUi ? m_nativeUi->SetButtonVisible(handle, visible) : false;
}

bool EngineCore::ConsumeNativeButtonPressed(NativeButtonHandle handle)
{
    return m_nativeUi ? m_nativeUi->ConsumeButtonPressed(handle) : false;
}

bool EngineCore::IsNativeButtonOpen(NativeButtonHandle handle) const
{
    return m_nativeUi ? m_nativeUi->IsButtonOpen(handle) : false;
}

double EngineCore::GetDeltaTime() const
{
    return static_cast<double>(m_context.DeltaTime);
}

bool EngineCore::IsWindowOpen() const
{
    return m_platform ? m_platform->IsWindowOpen() : false;
}

bool EngineCore::IsWindowActive() const
{
    return m_platform ? m_platform->IsWindowActive() : false;
}

std::wstring EngineCore::GetWindowTitle() const
{
    return m_platform ? m_platform->GetWindowTitle() : std::wstring();
}

bool EngineCore::IsWindowMaximized() const
{
    return m_platform ? m_platform->IsWindowMaximized() : false;
}

bool EngineCore::IsWindowMinimized() const
{
    return m_platform ? m_platform->IsWindowMinimized() : false;
}

HWND EngineCore::GetWindowHandle() const
{
    return m_platform ? m_platform->GetWindowHandle() : nullptr;
}

int EngineCore::GetWindowWidth() const
{
    return m_platform ? m_platform->GetWindowWidth() : 0;
}

int EngineCore::GetWindowHeight() const
{
    return m_platform ? m_platform->GetWindowHeight() : 0;
}

float EngineCore::GetWindowAspectRatio() const
{
    return m_platform ? m_platform->GetWindowAspectRatio() : 0.0f;
}

void EngineCore::SetWindowTitle(const std::wstring& title)
{
    if (m_platform)
    {
        m_platform->SetWindowTitle(title);
    }
}

void EngineCore::MaximizeWindow()
{
    if (m_platform)
    {
        m_platform->MaximizeWindow();
    }
}

void EngineCore::RestoreWindow()
{
    if (m_platform)
    {
        m_platform->RestoreWindow();
    }
}

void EngineCore::MinimizeWindow()
{
    if (m_platform)
    {
        m_platform->MinimizeWindow();
    }
}

void EngineCore::SetWindowSize(int width, int height)
{
    if (m_platform)
    {
        m_platform->SetWindowSize(width, height);
    }
}

void EngineCore::SetCursorVisible(bool visible)
{
    if (m_platform)
    {
        m_platform->SetCursorVisible(visible);
    }
}

bool EngineCore::IsCursorVisible() const
{
    return m_platform ? m_platform->IsCursorVisible() : true;
}

void EngineCore::SetCursorLocked(bool locked)
{
    if (m_platform)
    {
        m_platform->SetCursorLocked(locked);
    }
}

bool EngineCore::IsCursorLocked() const
{
    return m_platform ? m_platform->IsCursorLocked() : false;
}

bool EngineCore::IsAnyKeyPressed() const
{
    for (unsigned int key = 0; key < InputModule::KeyCount; ++key)
    {
        if (WasKeyPressed(static_cast<unsigned char>(key)))
        {
            return true;
        }
    }

    return false;
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

double EngineCore::GetMouseNormalizedX() const
{
    const int windowWidth = GetWindowWidth();
    if (windowWidth <= 0)
    {
        return 0.0;
    }

    return static_cast<double>(GetMouseX()) / static_cast<double>(windowWidth);
}

double EngineCore::GetMouseNormalizedY() const
{
    const int windowHeight = GetWindowHeight();
    if (windowHeight <= 0)
    {
        return 0.0;
    }

    return static_cast<double>(GetMouseY()) / static_cast<double>(windowHeight);
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

unsigned int EngineCore::GetIdleJobWorkerCount() const
{
    return m_jobs ? m_jobs->GetIdleWorkerCount() : 0U;
}

std::size_t EngineCore::GetQueuedJobCount() const
{
    return m_jobs ? m_jobs->GetQueuedJobCount() : 0;
}

std::size_t EngineCore::GetPeakQueuedJobCount() const
{
    return m_jobs ? m_jobs->GetPeakQueuedJobCount() : 0;
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

const MemoryStats& EngineCore::GetMemoryStats() const
{
    static const MemoryStats emptyStats{};
    return m_memory ? m_memory->GetStats() : emptyStats;
}

std::size_t EngineCore::GetMemoryBytesByClass(MemoryClass memoryClass) const
{
    return m_memory ? m_memory->GetBytesByClass(memoryClass) : 0;
}

std::size_t EngineCore::GetPeakMemoryBytesByClass(MemoryClass memoryClass) const
{
    return m_memory ? m_memory->GetPeakBytesByClass(memoryClass) : 0;
}

std::string EngineCore::BuildMemoryReport() const
{
    return m_memory ? m_memory->BuildReport() : std::string("Memory Report | unavailable");
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

bool EngineCore::HasPendingJobs() const
{
    return m_jobs ? m_jobs->HasPendingJobs() : false;
}

bool EngineCore::IsJobSystemIdle() const
{
    return m_jobs ? m_jobs->IsIdle() : true;
}

unsigned long long EngineCore::GetSubmittedJobCount() const
{
    return m_jobs ? m_jobs->GetSubmittedJobCount() : 0;
}

unsigned long long EngineCore::GetCompletedJobCount() const
{
    return m_jobs ? m_jobs->GetCompletedJobCount() : 0;
}

unsigned long long EngineCore::GetFailedJobCount() const
{
    return m_jobs ? m_jobs->GetFailedJobCount() : 0;
}

bool EngineCore::ApplySettingsFromText(const std::string& content)
{
    std::istringstream stream(content);
    std::string line;
    std::string sectionName;

    while (std::getline(stream, line))
    {
        const std::string trimmedLine = TrimString(line);
        if (trimmedLine.empty() || trimmedLine[0] == ';' || trimmedLine[0] == '#')
        {
            continue;
        }

        if (trimmedLine.front() == '[' && trimmedLine.back() == ']')
        {
            sectionName = TrimString(trimmedLine.substr(1, trimmedLine.size() - 2));
            continue;
        }

        const std::size_t separator = trimmedLine.find('=');
        if (separator == std::string::npos)
        {
            continue;
        }

        const std::string key = TrimString(trimmedLine.substr(0, separator));
        const std::string value = TrimString(trimmedLine.substr(separator + 1));

        if ((sectionName.empty() || sectionName == "core") && key == "fps_limit")
        {
            char* parseEnd = nullptr;
            const double parsedValue = std::strtod(value.c_str(), &parseEnd);
            if (parseEnd == value.c_str())
            {
                return false;
            }

            ApplyTargetFramesPerSecond(parsedValue > 0.0 ? parsedValue : m_targetFramesPerSecond);
        }
    }

    return true;
}

std::string EngineCore::BuildSettingsText() const
{
    std::ostringstream stream;
    stream << "; Engine-Chili2.0 core settings\n";
    stream << "[core]\n";
    stream << "fps_limit=" << m_fpsLimit << '\n';
    return stream.str();
}

std::wstring EngineCore::ToWideString(const std::string& value)
{
    return std::wstring(value.begin(), value.end());
}

void EngineCore::ApplyTargetFramesPerSecond(double framesPerSecond)
{
    if (framesPerSecond < 15.0)
    {
        framesPerSecond = 15.0;
    }
    else if (framesPerSecond > 480.0)
    {
        framesPerSecond = 480.0;
    }

    m_fpsLimit = framesPerSecond;
    m_targetFramesPerSecond = framesPerSecond;
    m_targetFrameTime = (m_targetFramesPerSecond > 0.0)
        ? (1.0 / m_targetFramesPerSecond)
        : 0.0;
}

void EngineCore::UpdateFramePacing(double frameDurationSeconds)
{
    if (frameDurationSeconds < 0.0)
    {
        frameDurationSeconds = 0.0;
    }

    m_lastFrameDuration = frameDurationSeconds;

    double lateness = frameDurationSeconds - m_targetFrameTime;
    if (lateness < 0.0)
    {
        lateness = 0.0;
    }

    m_lastFrameLateness = lateness;
    m_isBehindSchedule = (lateness > 0.0);

    if (m_isBehindSchedule)
    {
        ++m_lateFrameCount;

        if (lateness > m_maxFrameLateness)
        {
            m_maxFrameLateness = lateness;
        }
    }

    if (m_targetFrameTime <= 0.0)
    {
        return;
    }

    const double timingError = frameDurationSeconds - m_targetFrameTime;

    if (timingError > 0.0)
    {
        // Overshooting means our coarse/fine wait margins are too aggressive for current machine timing.
        m_adaptiveSleepMargin = (m_adaptiveSleepMargin * 0.90) + (timingError * 0.10);
        m_adaptiveYieldMargin = (m_adaptiveYieldMargin * 0.92) + (timingError * 0.08);
    }
    else
    {
        // Arriving early means we can slowly reclaim some CPU by shrinking margins.
        m_adaptiveSleepMargin = (m_adaptiveSleepMargin * 0.995) + (0.0010 * 0.005);
        m_adaptiveYieldMargin = (m_adaptiveYieldMargin * 0.995) + (0.0002 * 0.005);
    }

    if (m_adaptiveSleepMargin < 0.0005)
    {
        m_adaptiveSleepMargin = 0.0005;
    }
    else if (m_adaptiveSleepMargin > 0.0050)
    {
        m_adaptiveSleepMargin = 0.0050;
    }

    if (m_adaptiveYieldMargin < 0.00005)
    {
        m_adaptiveYieldMargin = 0.00005;
    }
    else if (m_adaptiveYieldMargin > 0.0015)
    {
        m_adaptiveYieldMargin = 0.0015;
    }

    if (m_adaptiveYieldMargin > m_adaptiveSleepMargin * 0.8)
    {
        m_adaptiveYieldMargin = m_adaptiveSleepMargin * 0.8;
    }
}
