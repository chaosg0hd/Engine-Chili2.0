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
#include "../modules/webview/webview_module.hpp"

#include <windows.h>

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <sstream>
#include <string>

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
    m_platform = m_modules.AddModule<PlatformModule>();
    m_render = m_modules.AddModule<RenderModule>();
    m_input = m_modules.AddModule<InputModule>();
    m_jobs = m_modules.AddModule<JobModule>();
    m_memory = m_modules.AddModule<MemoryModule>();
    m_files = m_modules.AddModule<FileModule>();
    m_gpuCompute = m_modules.AddModule<GpuComputeModule>();
    m_webViews = m_modules.AddModule<WebViewModule>();

    if (m_render)
    {
        m_render->SetPlatformModule(m_platform);
    }

    if (m_input)
    {
        m_input->SetPlatformModule(m_platform);
    }

    if (m_webViews)
    {
        m_webViews->SetPlatformModule(m_platform);
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
    m_overlayDirty = true;
    m_lastFrameDuration = 0.0;
    m_lastFrameLateness = 0.0;
    m_maxFrameLateness = 0.0;
    m_lateFrameCount = 0;
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
    m_webViews = nullptr;

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

std::wstring EngineCore::BuildDebugViewText() const
{
    const double deltaTime = GetDeltaTime();
    const double framesPerSecond = (deltaTime > 0.0) ? (1.0 / deltaTime) : 0.0;
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
        L"  Last Frame Lateness: " + std::to_wstring(GetLastFrameLateness()) + L"s\n"
        L"  Max Frame Lateness: " + std::to_wstring(GetMaxFrameLateness()) + L"s\n"
        L"  Late Frame Count: " + std::to_wstring(GetLateFrameCount()) + L"\n"
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
        L"  Workers: " + std::to_wstring(GetJobWorkerCount()) + L"\n"
        L"  Queued: " + std::to_wstring(GetQueuedJobCount()) + L"\n"
        L"  Active: " + std::to_wstring(GetActiveJobCount()) + L"\n"
        L"\n"
        L"Memory\n"
        L"  Current Bytes: " + std::to_wstring(GetCurrentMemoryBytes()) + L"\n"
        L"  Peak Bytes: " + std::to_wstring(GetPeakMemoryBytes()) + L"\n"
        L"  Allocations: " + std::to_wstring(GetMemoryAllocationCount()) + L"\n"
        L"  Frees: " + std::to_wstring(GetMemoryFreeCount()) + L"\n"
        L"\n"
        L"System\n"
        L"  Settings: " + ToWideString(GetSettingsPath()) + L"\n"
        L"  Settings Abs: " + ToWideString(GetAbsolutePath(GetSettingsPath())) + L"\n"
        L"  Working Dir: " + ToWideString(GetWorkingDirectory()) + L"\n"
        L"  Window Open/Active: " + std::wstring(IsWindowOpen() ? L"Yes" : L"No") + L" / " + std::wstring(IsWindowActive() ? L"Yes" : L"No") + L"\n"
        L"  Window Size: " + std::to_wstring(GetWindowWidth()) + L" x " + std::to_wstring(GetWindowHeight()) + L"\n"
        L"  GPU Backend: " + ToWideString(GetGpuBackendName()) + L"\n"
        L"  GPU Available: " + std::wstring(IsGpuComputeAvailable() ? L"Yes" : L"No") + L"\n"
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

    if (m_platform && m_overlayEnabled)
    {
        m_platform->SetOverlayText(m_appOverlayText);
        m_overlayDirty = false;
        m_nextOverlayRefreshTime = m_context.TotalTime + 0.50;
    }
}

void EngineCore::SetWindowOverlayText(const std::wstring& text)
{
    if (m_appOverlayText == text)
    {
        return;
    }

    m_appOverlayText = text;
    m_overlayDirty = true;

    if (m_platform && m_overlayEnabled)
    {
        m_platform->SetOverlayText(m_appOverlayText);
        m_overlayDirty = false;
        m_nextOverlayRefreshTime = m_context.TotalTime + 0.50;
    }
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

bool EngineCore::IsBehindSchedule() const
{
    return m_isBehindSchedule;
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

bool EngineCore::SupportsGpuBuffers() const
{
    return m_gpuCompute ? m_gpuCompute->SupportsGpuBuffers() : false;
}

bool EngineCore::SupportsComputeDispatch() const
{
    return m_gpuCompute ? m_gpuCompute->SupportsComputeDispatch() : false;
}

std::string EngineCore::GetGpuCapabilitySummary() const
{
    return m_gpuCompute ? m_gpuCompute->GetCapabilitySummary() : std::string("backend=Unavailable | available=false | buffers=false | dispatch=false");
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

const MemoryStats& EngineCore::GetMemoryStats() const
{
    static const MemoryStats emptyStats{};
    return m_memory ? m_memory->GetStats() : emptyStats;
}

std::size_t EngineCore::GetMemoryBytesByClass(MemoryClass memoryClass) const
{
    return m_memory ? m_memory->GetBytesByClass(memoryClass) : 0;
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
}
