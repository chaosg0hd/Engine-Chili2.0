# Engine Core Documentation

## Overview

`engine_core` is the native runtime layer for Engine-Chili2.0.
It boots the engine modules, owns the frame lifecycle, runs the Win32 platform window,
tracks timing and diagnostics, and exposes the app-facing API through `EngineCore`.

The current sandbox execution path is:

1. `main()` creates `App`
2. `App::Run()` creates `EngineCore`
3. `EngineCore::Initialize()` creates and initializes engine modules
4. `App` runs feature tests through `EngineCore`
5. `EngineCore::Run()` enters the runtime loop
6. `EngineCore::Shutdown()` shuts modules down in reverse order

## Structure

### Entry Layer

- `src/main.cpp`
  - Creates `App`
  - Calls `App::Run()`
- `src/app/app.hpp`
- `src/app/app.cpp`
  - Holds the current feature-test app harness
  - Runs startup checks and engine feature tests
  - Enters the engine runtime loop

### Core Layer

- `src/core/engine_core.hpp`
- `src/core/engine_core.cpp`
  - Main engine-facing API
  - Owns the module manager
  - Coordinates runtime flow
- `src/core/engine_context.hpp`
  - Shared state passed to modules
- `src/core/module.hpp`
  - Base interface for modules
- `src/core/module_manager.hpp`
- `src/core/module_manager.cpp`
  - Handles module initialize/start/update/shutdown ordering

### Module Order

`EngineCore::Initialize()` currently adds modules in this order:

1. `LoggerModule`
2. `TimerModule`
3. `DiagnosticsModule`
4. `PlatformModule`
5. `RenderModule`
6. `InputModule`
7. `JobModule`
8. `MemoryModule`
9. `FileModule`
10. `GpuComputeModule`

That means initialization and startup follow this order, while shutdown is reversed.

## Runtime Flow

Inside `EngineCore::Run()`, the engine loops while `EngineContext::IsRunning` is true.

Each frame currently calls:

1. `BeginFrame()`
2. `ServicePlatform()`
3. `DispatchAsyncWork()`
4. `ServiceCoreWork()`
5. `ProcessEvents()`
6. `ProcessCompletedWork()`
7. `ServiceScheduledWork()`
8. `ServiceDeferredWork()`
9. `SynchronizeCriticalWork()`
10. `EndFrame()`

`EngineCore::Tick()` runs the same per-frame path for hosts, such as the studio backend, that need to keep the native engine responsive without entering `EngineCore::Run()`.

Important current behavior:

- timer updates `EngineContext::DeltaTime` and `EngineContext::TotalTime`
- diagnostics updates loop count and uptime
- platform polls Win32 messages every frame
- scheduled work refreshes the overlay text and emits diagnostics periodically
- pressing `Escape` requests shutdown
- closing the window requests shutdown

## Current Features

- logging through `LoggerModule`
- timing through `TimerModule`
- core settings loaded from `config/engine.ini`
- diagnostics tracking through `DiagnosticsModule`
- Win32 window creation and event polling through `PlatformModule` and `PlatformWindow`
- software backbuffer rendering through `RenderModule`
- in-window GDI overlay text
- background job dispatch through `JobModule`
- keyboard and mouse state queries through `InputModule`
- raw and typed memory allocation through `MemoryModule`
- file and directory utilities through `FileModule`
- GPU compute capability queries through `GpuComputeModule`

## Public API Inventory

### `App`

File: `src/app/app.hpp`

- `bool Run()`

Private helpers used by the current app harness:

- `bool RunStartupChecks(EngineCore& core)`
- `bool RunMemoryFeatureTest(EngineCore& core)`
- `bool RunRawMemoryFeatureTest(EngineCore& core)`
- `bool RunFileFeatureTest(EngineCore& core)`
- `bool RunGpuFeatureTest(EngineCore& core)`
- `bool RunJobFeatureTest(EngineCore& core)`
- `bool RunInputFeatureTest(EngineCore& core)`
- `void UpdateFrame(EngineCore& core)`
- `void UpdateDisplayToggle(EngineCore& core)`
- `void RunTextOverlayMode(EngineCore& core)`
- `void RunPixelRendererMode(EngineCore& core)`
- `void RebuildStarField(int width, int height)`
- `void LogFeatureSummary(EngineCore& core) const`

### `EngineCore`

File: `src/core/engine_core.hpp`

Lifecycle:

- `EngineCore()`
- `bool Initialize()`
- `bool Run()`
- `bool Tick()`
- `void Shutdown()`
- `void RequestShutdown()`

Logging:

- `void LogInfo(const std::string& message)`
- `void LogWarn(const std::string& message)`
- `void LogError(const std::string& message)`
- `std::wstring BuildDebugViewText() const`
- `void ShowDebugView()`

Window overlay:

- `void ClearWindowOverlayText()`
- `bool IsWindowOverlayEnabled() const`
- `void SetWindowOverlayEnabled(bool enabled)`
- `void AppendWindowOverlayText(const std::wstring& text)`
- `void SetWindowOverlayText(const std::wstring& text)`
- `void SetFrameCallback(FrameCallback callback)`

Settings:

- `bool LoadSettings()`
- `bool LoadSettings(const std::string& path)`
- `bool SaveSettings() const`
- `bool SaveSettings(const std::string& path) const`
- `void ResetSettingsToDefaults()`
- `const std::string& GetSettingsPath() const`
- `double GetFpsLimit() const`
- `void SetFpsLimit(double framesPerSecond)`
- `void SetTargetFramesPerSecond(double framesPerSecond)`
- `double GetTargetFramesPerSecond() const`
- `double GetTargetFrameTime() const`

Rendering:

- `void ClearFrame(std::uint32_t color)`
- `void PutFramePixel(int x, int y, std::uint32_t color)`
- `void DrawFrameLine(int x0, int y0, int x1, int y1, std::uint32_t color)`
- `void DrawFrameRect(int x, int y, int width, int height, std::uint32_t color)`
- `void FillFrameRect(int x, int y, int width, int height, std::uint32_t color)`
- `void DrawFrameGrid(int cellSize, std::uint32_t color)`
- `void DrawFrameCrosshair(int x, int y, int size, std::uint32_t color)`
- `void PresentFrame()`
- `int GetFrameWidth() const`
- `int GetFrameHeight() const`
- `double GetFrameAspectRatio() const`

Files:

- `bool FileExists(const std::string& path) const`
- `bool DirectoryExists(const std::string& path) const`
- `bool CreateDirectory(const std::string& path)`
- `bool WriteTextFile(const std::string& path, const std::string& content)`
- `bool ReadTextFile(const std::string& path, std::string& outContent) const`
- `bool WriteBinaryFile(const std::string& path, const std::vector<std::byte>& content)`
- `bool ReadBinaryFile(const std::string& path, std::vector<std::byte>& outContent) const`
- `bool DeleteFile(const std::string& path)`
- `std::uintmax_t GetFileSize(const std::string& path) const`
- `std::string GetWorkingDirectory() const`
- `std::vector<std::string> ListDirectory(const std::string& path) const`
- `std::vector<std::string> ListFiles(const std::string& path) const`
- `std::vector<std::string> ListDirectories(const std::string& path) const`
- `std::string GetAbsolutePath(const std::string& path) const`
- `std::string NormalizePath(const std::string& path) const`
- `bool CopyFile(const std::string& source, const std::string& destination)`
- `bool MoveFile(const std::string& source, const std::string& destination)`

GPU compute:

- `bool IsGpuComputeAvailable() const`
- `bool SubmitGpuTask(const GpuTaskDesc& task)`
- `void WaitForGpuIdle()`
- `std::string GetGpuBackendName() const`
- `bool SupportsGpuBuffers() const`
- `bool SupportsComputeDispatch() const`
- `std::string GetGpuCapabilitySummary() const`

Input:

- `double GetDeltaTime() const`
- `bool IsWindowOpen() const`
- `bool IsWindowActive() const`
- `std::wstring GetWindowTitle() const`
- `bool IsWindowMaximized() const`
- `bool IsWindowMinimized() const`
- `int GetWindowWidth() const`
- `int GetWindowHeight() const`
- `float GetWindowAspectRatio() const`
- `void SetWindowTitle(const std::wstring& title)`
- `void MaximizeWindow()`
- `void RestoreWindow()`
- `void MinimizeWindow()`
- `void SetWindowSize(int width, int height)`
- `void SetCursorVisible(bool visible)`
- `bool IsCursorVisible() const`
- `void SetCursorLocked(bool locked)`
- `bool IsCursorLocked() const`
- `bool IsAnyKeyPressed() const`
- `bool IsKeyDown(unsigned char key) const`
- `bool WasKeyPressed(unsigned char key) const`
- `bool WasKeyReleased(unsigned char key) const`
- `bool IsMouseButtonDown(InputModule::MouseButton button) const`
- `bool WasMouseButtonPressed(InputModule::MouseButton button) const`
- `bool WasMouseButtonReleased(InputModule::MouseButton button) const`
- `int GetMouseX() const`
- `int GetMouseY() const`
- `int GetMouseDeltaX() const`
- `int GetMouseDeltaY() const`
- `int GetMouseWheelDelta() const`
- `double GetMouseNormalizedX() const`
- `double GetMouseNormalizedY() const`

Jobs:

- `bool SubmitJob(JobFunction job)`
- `void WaitForAllJobs()`
- `unsigned int GetJobWorkerCount() const`
- `std::size_t GetQueuedJobCount() const`
- `unsigned int GetActiveJobCount() const`

Memory:

- `void* AllocateMemory(std::size_t size, MemoryClass memoryClass, std::size_t alignment = alignof(std::max_align_t), const char* owner = nullptr)`
- `void FreeMemory(void* ptr)`
- `template<typename T, typename... Args> T* NewObject(MemoryClass memoryClass, const char* owner, Args&&... args)`
- `template<typename T> void DeleteObject(T* ptr)`
- `template<typename T> T* NewArray(std::size_t count, MemoryClass memoryClass, const char* owner = nullptr)`
- `template<typename T> void DeleteArray(T* ptr, std::size_t count)`
- `std::size_t GetCurrentMemoryBytes() const`
- `std::size_t GetPeakMemoryBytes() const`
- `std::size_t GetMemoryAllocationCount() const`
- `std::size_t GetMemoryFreeCount() const`
- `const MemoryStats& GetMemoryStats() const`
- `std::size_t GetMemoryBytesByClass(MemoryClass memoryClass) const`
- `std::string BuildMemoryReport() const`

Timing and diagnostics:

- `double GetTotalTime() const`
- `double GetDiagnosticsUptime() const`
- `unsigned long long GetDiagnosticsLoopCount() const`
- `bool HasPendingJobs() const`
- `bool IsJobSystemIdle() const`
- `unsigned long long GetSubmittedJobCount() const`
- `unsigned long long GetCompletedJobCount() const`

### Core Support Types

- `EngineContext` in `src/core/engine_context.hpp`
- `IModule` in `src/core/module.hpp`
- `ModuleManager` in `src/core/module_manager.hpp`

### Module APIs

The module API inventory lives in the module headers under `src/modules/`:

- `logger/logger_module.hpp`
- `timer/timer_module.hpp`
- `diagnostics/diagnostics_module.hpp`
- `platform/platform_module.hpp`
- `platform/platform_window.hpp`
- `jobs/job_module.hpp`
- `render/render_module.hpp`
- `input/input_module.hpp`
- `file/file_module.hpp`
- `gpu/gpu_compute_module.hpp`
- `memory/memory_module.hpp`

## Data Paths

### Overlay Text Path

1. `App` calls `EngineCore::ShowDebugView()` or `SetWindowOverlayText(...)`
2. `EngineCore` stores or builds overlay text
3. `EngineCore::ServiceScheduledWork()` forwards it to platform
4. `PlatformModule::SetOverlayText(...)`
5. `PlatformWindow::SetOverlayText(...)`
6. `InvalidateRect(...)`
7. `WM_PAINT`
8. `PlatformWindow::DrawOverlayText(HDC dc)`
9. `DrawTextW(...)`

### Render Path

1. `App` drives rendering through the per-frame callback
2. `EngineCore` forwards rendering calls to `RenderModule`
3. `RenderModule` resizes the software backbuffer to the client area
4. Pixels are written into a CPU-side `std::vector<std::uint32_t>`
5. `Present()` blits the backbuffer with `StretchDIBits`

### Input Path

1. `PlatformWindow` captures Win32 keyboard and mouse messages
2. `PlatformModule` exposes those events
3. `InputModule::Update()` reads platform events each frame
4. `InputModule` stores per-frame state
5. `EngineCore` exposes input queries to apps and hosts

### File Path

1. App or engine code calls an `EngineCore` file wrapper
2. `EngineCore` forwards the request to `FileModule`
3. `FileModule` uses `std::filesystem` and file streams
4. Results are returned as success or failure

### Settings Path

1. `EngineCore::Initialize()` loads `config/engine.ini`
2. If the file does not exist, core writes a default settings file
3. Core currently applies `fps_limit`
4. `EndFrame()` uses the FPS limit to pace the frame

Current shipped setting:

```ini
[core]
fps_limit=60
```

### GPU Compute Path

1. App or engine code builds a `GpuTaskDesc`
2. `EngineCore` forwards submission to `GpuComputeModule`
3. `GpuComputeModule` validates the descriptor
4. The current backend reports `Stub`
5. Submission fails cleanly when compute is unavailable

## Build Notes

Current helper scripts:

```powershell
.\clean.cmd
.\configure.cmd
.\build.cmd
.\rebuild.cmd
```

VS Code task automation maps to the same flow:

- `Clean Build State`
- `CMake Configure`
- `CMake Build`
- `Rebuild Engine`

CI automation:

- `.github/workflows/windows-build.yml`

## Current Gaps

- leak reporting in `MemoryModule`
- class-by-class memory stats reporting
- more engine settings beyond `fps_limit`
- a real GPU compute backend behind `GpuComputeModule`
- richer app-side feature scenarios
- clearer separation between app-facing APIs and internal-only module APIs
