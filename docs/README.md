# Engine-Chili2.0 Documentation

## Overview

This project is a modular C++ engine sandbox built around a small central runtime called `EngineCore`.
The current engine boots a set of modules, runs a Win32 window loop, tracks timing and diagnostics,
supports async job dispatch, and exposes a memory service with typed allocation helpers.

The current execution path is:

1. `main()` creates `App`
2. `App::Run()` creates `EngineCore`
3. `EngineCore::Initialize()` creates and initializes engine modules
4. `App` runs feature tests through `EngineCore`
5. `EngineCore::Run()` enters the runtime loop
6. `EngineCore::Shutdown()` shuts modules down in reverse order

## Current Engine Structure

### Entry Layer

- `src/main.cpp`
  - Creates `App`
  - Calls `App::Run()`

- `src/app/app.hpp`
- `src/app/app.cpp`
  - Holds the current feature-test app harness
  - Runs startup checks
  - Runs memory feature tests
  - Runs job feature tests
  - Enters the engine runtime loop

### Core Layer

- `src/core/engine_core.hpp`
- `src/core/engine_core.cpp`
  - Main engine-facing API
  - Owns the module manager
  - Holds direct pointers to key modules
  - Coordinates runtime flow

- `src/core/engine_context.hpp`
  - Shared state passed to all modules
  - Current fields:
    - `bool IsRunning`
    - `float DeltaTime`
    - `double TotalTime`

- `src/core/module.hpp`
  - Base interface for all modules

- `src/core/module_manager.hpp`
- `src/core/module_manager.cpp`
  - Stores modules
  - Initializes in insertion order
  - Starts in insertion order
  - Updates in insertion order
  - Shuts down in reverse order

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

That means:

- initialization order is the same as above
- startup order is the same as above
- shutdown order is reversed

## Runtime Flow

Inside `EngineCore::Run()`, the engine loops while `EngineContext::IsRunning` is true.

Each iteration currently calls:

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

Important current behavior:

- `BeginFrame()` updates timer state first and clamps bad delta spikes
- `DispatchAsyncWork()` updates modules using the current frame delta from `EngineContext`
- timer updates `EngineContext::DeltaTime` and `EngineContext::TotalTime`
- diagnostics updates loop count and uptime
- platform polls Win32 messages
- scheduled work refreshes the window overlay on a short interval and emits diagnostics periodically
- pressing Escape requests shutdown
- closing the window requests shutdown

## Current Features

The engine currently has these working feature areas:

- logging through `LoggerModule`
- timing through `TimerModule`
- core settings loaded from `config/engine.ini`
- diagnostics tracking through `DiagnosticsModule`
- Win32 window creation and event polling through `PlatformModule` and `PlatformWindow`
- software backbuffer rendering through `RenderModule`
- in-window GDI overlay text at the top-left of the client area
- background job dispatch and idle waiting through `JobModule`
- keyboard and mouse state queries through `InputModule`
- raw and typed memory allocation through `MemoryModule`
- file and directory utilities through `FileModule`
- GPU compute capability queries through `GpuComputeModule`
- basic feature-test app flow through `App`

## Public API Inventory

This section lists the engine API surfaces that exist right now.

### `App`

File: `src/app/app.hpp`

- `bool Run()`

Private helper methods used by the current app harness:

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
- `void Shutdown()`
- `void RequestShutdown()`

Logging:

- `void LogInfo(const std::string& message)`
- `void LogWarn(const std::string& message)`
- `void LogError(const std::string& message)`

Window overlay:

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

Rendering:

- `void ClearFrame(std::uint32_t color)`
- `void PutFramePixel(int x, int y, std::uint32_t color)`
- `void PresentFrame()`
- `int GetFrameWidth() const`
- `int GetFrameHeight() const`

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

GPU compute:

- `bool IsGpuComputeAvailable() const`
- `bool SubmitGpuTask(const GpuTaskDesc& task)`
- `void WaitForGpuIdle()`
- `std::string GetGpuBackendName() const`

Input:

- `double GetDeltaTime() const`
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

Jobs:

- `bool SubmitJob(JobFunction job)`
- `void WaitForAllJobs()`
- `unsigned int GetJobWorkerCount() const`
- `std::size_t GetQueuedJobCount() const`
- `unsigned int GetActiveJobCount() const`

Memory, raw:

- `void* AllocateMemory(std::size_t size, MemoryClass memoryClass, std::size_t alignment = alignof(std::max_align_t), const char* owner = nullptr)`
- `void FreeMemory(void* ptr)`

Memory, typed:

- `template<typename T, typename... Args> T* NewObject(MemoryClass memoryClass, const char* owner, Args&&... args)`
- `template<typename T> void DeleteObject(T* ptr)`
- `template<typename T> T* NewArray(std::size_t count, MemoryClass memoryClass, const char* owner = nullptr)`
- `template<typename T> void DeleteArray(T* ptr, std::size_t count)`

Memory stats:

- `std::size_t GetCurrentMemoryBytes() const`
- `std::size_t GetPeakMemoryBytes() const`
- `std::size_t GetMemoryAllocationCount() const`
- `std::size_t GetMemoryFreeCount() const`

Timing and diagnostics:

- `double GetTotalTime() const`
- `double GetDiagnosticsUptime() const`
- `unsigned long long GetDiagnosticsLoopCount() const`

Internal runtime helpers inside `EngineCore`:

- `void BeginFrame()`
- `void ServicePlatform()`
- `void DispatchAsyncWork()`
- `void ServiceCoreWork()`
- `void ProcessEvents()`
- `void ProcessCompletedWork()`
- `void ServiceScheduledWork()`
- `void ServiceDeferredWork()`
- `void SynchronizeCriticalWork()`
- `void EndFrame()`
- `void ProcessPlatformEvents()`
- `void HandleWindowStateChanges()`
- `void EmitScheduledDiagnostics()`

### `EngineContext`

File: `src/core/engine_context.hpp`

Fields:

- `bool IsRunning`
- `float DeltaTime`
- `double TotalTime`

### `IModule`

File: `src/core/module.hpp`

- `virtual ~IModule() = default`
- `virtual const char* GetName() const = 0`
- `virtual bool Initialize(EngineContext& context) = 0`
- `virtual void Startup(EngineContext& context)`
- `virtual void Update(EngineContext& context, float deltaTime)`
- `virtual void Shutdown(EngineContext& context) = 0`

### `ModuleManager`

Files:

- `src/core/module_manager.hpp`
- `src/core/module_manager.cpp`

Methods:

- `template<typename T> T* AddModule()`
- `bool InitializeAll(EngineContext& context)`
- `void StartupAll(EngineContext& context)`
- `void UpdateAll(EngineContext& context, float deltaTime)`
- `void ShutdownAll(EngineContext& context)`

## Module API Inventory

### `LoggerModule`

File: `src/modules/logger/logger_module.hpp`

- `const char* GetName() const override`
- `bool Initialize(EngineContext& context) override`
- `void Startup(EngineContext& context) override`
- `void Shutdown(EngineContext& context) override`
- `void Info(const std::string& message)`
- `void Warn(const std::string& message)`
- `void Error(const std::string& message)`
- `bool IsInitialized() const`
- `bool IsStarted() const`

### `TimerModule`

File: `src/modules/timer/timer_module.hpp`

- `TimerModule()`
- `const char* GetName() const override`
- `bool Initialize(EngineContext& context) override`
- `void Startup(EngineContext& context) override`
- `void Update(EngineContext& context, float deltaTime) override`
- `void Shutdown(EngineContext& context) override`
- `double GetDeltaTime() const`
- `double GetTotalTime() const`
- `bool IsInitialized() const`
- `bool IsStarted() const`

Private helpers:

- `void Reset()`
- `void Tick()`
- `static long long GetCurrentTicks()`
- `static double TicksToSeconds(long long ticks)`

### `DiagnosticsModule`

File: `src/modules/diagnostics/diagnostics_module.hpp`

- `DiagnosticsModule()`
- `const char* GetName() const override`
- `bool Initialize(EngineContext& context) override`
- `void Startup(EngineContext& context) override`
- `void Update(EngineContext& context, float deltaTime) override`
- `void Shutdown(EngineContext& context) override`
- `void EmitReport() const`
- `double GetUptimeSeconds() const`
- `unsigned long long GetLoopCount() const`
- `bool IsInitialized() const`
- `bool IsStarted() const`

### `PlatformModule`

File: `src/modules/platform/platform_module.hpp`

- `PlatformModule()`
- `~PlatformModule()`
- `const char* GetName() const override`
- `bool Initialize(EngineContext& context) override`
- `void Startup(EngineContext& context) override`
- `void Update(EngineContext& context, float deltaTime) override`
- `void Shutdown(EngineContext& context) override`
- `bool IsInitialized() const`
- `bool IsStarted() const`
- `bool IsWindowOpen() const`
- `bool IsWindowActive() const`
- `HWND GetWindowHandle() const`
- `void PollEvents()`
- `const std::vector<PlatformWindow::Event>& GetEvents() const`
- `void ClearEvents()`
- `const std::string& GetPlatformName() const`
- `void SetOverlayText(const std::wstring& text)`
- `const std::wstring& GetOverlayText() const`

### `PlatformWindow`

File: `src/modules/platform/platform_window.hpp`

Window events:

- `PlatformWindow::Event::None`
- `PlatformWindow::Event::WindowClosed`
- `PlatformWindow::Event::WindowActivated`
- `PlatformWindow::Event::WindowDeactivated`
- `PlatformWindow::Event::KeyDown`
- `PlatformWindow::Event::KeyUp`
- `PlatformWindow::Event::MouseMove`
- `PlatformWindow::Event::MouseButtonDown`
- `PlatformWindow::Event::MouseButtonUp`
- `PlatformWindow::Event::MouseWheel`

Window methods:

- `PlatformWindow()`
- `~PlatformWindow()`
- `bool Create(const wchar_t* title, int clientWidth, int clientHeight)`
- `void Show(int cmdShow)`
- `void PollEvents()`
- `bool IsOpen() const`
- `bool IsActive() const`
- `HWND GetHandle() const`
- `const std::vector<Event>& GetEvents() const`
- `void ClearEvents()`
- `void SetOverlayText(const std::wstring& text)`
- `const std::wstring& GetOverlayText() const`

Private window message helpers:

- `static LRESULT CALLBACK WindowProcSetup(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)`
- `static LRESULT CALLBACK WindowProcThunk(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)`
- `LRESULT HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)`
- `void DrawOverlayText(HDC dc)`

### `JobModule`

File: `src/modules/jobs/job_module.hpp`

- `using JobFunction = std::function<void()>`
- `JobModule()`
- `~JobModule() override`
- `const char* GetName() const override`
- `bool Initialize(EngineContext& context) override`
- `void Startup(EngineContext& context) override`
- `void Update(EngineContext& context, float deltaTime) override`
- `void Shutdown(EngineContext& context) override`
- `void Submit(JobFunction job)`
- `void WaitIdle()`
- `unsigned int GetWorkerCount() const`
- `std::size_t GetQueuedJobCount() const`
- `unsigned int GetActiveJobCount() const`
- `bool IsInitialized() const`
- `bool IsStarted() const`

Private helpers:

- `void StartWorkers()`
- `void StopWorkers()`
- `void WorkerLoop()`
- `unsigned int DetermineWorkerCount() const`

### `RenderModule`

File: `src/modules/render/render_module.hpp`

- `RenderModule()`
- `const char* GetName() const override`
- `bool Initialize(EngineContext& context) override`
- `void Startup(EngineContext& context) override`
- `void Update(EngineContext& context, float deltaTime) override`
- `void Shutdown(EngineContext& context) override`
- `void SetPlatformModule(PlatformModule* platform)`
- `bool ResizeToClientArea()`
- `void Clear(std::uint32_t color)`
- `void PutPixel(int x, int y, std::uint32_t color)`
- `void Present()`
- `int GetBackbufferWidth() const`
- `int GetBackbufferHeight() const`
- `bool IsInitialized() const`
- `bool IsStarted() const`

### `InputModule`

File: `src/modules/input/input_module.hpp`

- `static constexpr std::size_t KeyCount = 256`
- `enum class MouseButton`
- `InputModule()`
- `const char* GetName() const override`
- `bool Initialize(EngineContext& context) override`
- `void Startup(EngineContext& context) override`
- `void Update(EngineContext& context, float deltaTime) override`
- `void Shutdown(EngineContext& context) override`
- `void SetPlatformModule(PlatformModule* platform)`
- `bool IsKeyDown(std::uint8_t key) const`
- `bool WasKeyPressed(std::uint8_t key) const`
- `bool WasKeyReleased(std::uint8_t key) const`
- `bool IsMouseButtonDown(MouseButton button) const`
- `bool WasMouseButtonPressed(MouseButton button) const`
- `bool WasMouseButtonReleased(MouseButton button) const`
- `int GetMouseX() const`
- `int GetMouseY() const`
- `int GetMouseDeltaX() const`
- `int GetMouseDeltaY() const`
- `int GetMouseWheelDelta() const`
- `bool IsInitialized() const`
- `bool IsStarted() const`

### `FileModule`

File: `src/modules/file/file_module.hpp`

- `const char* GetName() const override`
- `bool Initialize(EngineContext& context) override`
- `void Startup(EngineContext& context) override`
- `void Update(EngineContext& context, float deltaTime) override`
- `void Shutdown(EngineContext& context) override`
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
- `bool IsInitialized() const`
- `bool IsStarted() const`

### `GpuComputeModule`

File: `src/modules/gpu/gpu_compute_module.hpp`

Support types:

- `struct GpuBufferView`
- `struct MutableGpuBufferView`
- `struct GpuTaskDesc`

Methods:

- `const char* GetName() const override`
- `bool Initialize(EngineContext& context) override`
- `void Startup(EngineContext& context) override`
- `void Update(EngineContext& context, float deltaTime) override`
- `void Shutdown(EngineContext& context) override`
- `bool IsGpuComputeAvailable() const`
- `bool SubmitGpuTask(const GpuTaskDesc& task)`
- `void WaitForGpuIdle()`
- `const char* GetBackendName() const`
- `bool IsInitialized() const`
- `bool IsStarted() const`

### `MemoryModule`

File: `src/modules/memory/memory_module.hpp`

Memory classes:

- `MemoryClass::Unknown`
- `MemoryClass::Core`
- `MemoryClass::Module`
- `MemoryClass::Frame`
- `MemoryClass::Resource`
- `MemoryClass::Persistent`
- `MemoryClass::Temporary`
- `MemoryClass::Job`
- `MemoryClass::Debug`
- `MemoryClass::Count`

Support types:

- `struct MemoryStats`
- `struct AllocationRecord`

Methods:

- `const char* GetName() const override`
- `bool Initialize(EngineContext& context) override`
- `void Startup(EngineContext& context) override`
- `void Update(EngineContext& context, float deltaTime) override`
- `void Shutdown(EngineContext& context) override`
- `void* Allocate(std::size_t size, MemoryClass memoryClass, std::size_t alignment = alignof(std::max_align_t), const char* owner = nullptr)`
- `void Free(void* ptr)`
- `template<typename T, typename... Args> T* New(MemoryClass memoryClass, const char* owner, Args&&... args)`
- `template<typename T> void Delete(T* ptr)`
- `template<typename T> T* NewArray(std::size_t count, MemoryClass memoryClass, const char* owner = nullptr)`
- `template<typename T> void DeleteArray(T* ptr, std::size_t count)`
- `const MemoryStats& GetStats() const`
- `std::size_t GetCurrentBytes() const`
- `std::size_t GetPeakBytes() const`
- `std::size_t GetAllocationCount() const`
- `std::size_t GetFreeCount() const`
- `bool IsInitialized() const`
- `bool IsStarted() const`
- `static const char* ToString(MemoryClass memoryClass)`

Private helper:

- `void ResetTracking()`

## What The App Uses Right Now

The current app is a feature-test harness and uses these `EngineCore` calls directly:

Lifecycle:

- `Initialize()`
- `Run()`
- `Shutdown()`

Logging:

- `LogInfo()`
- `LogWarn()`
- `LogError()`

Files:

- `CreateDirectory()`
- `WriteTextFile()`
- `ReadTextFile()`
- `WriteBinaryFile()`
- `ReadBinaryFile()`
- `FileExists()`
- `GetFileSize()`
- `GetWorkingDirectory()`

GPU compute:

- `IsGpuComputeAvailable()`
- `SubmitGpuTask()`
- `WaitForGpuIdle()`
- `GetGpuBackendName()`

Jobs:

- `SubmitJob()`
- `WaitForAllJobs()`
- `GetJobWorkerCount()`

Input:

- `IsKeyDown()`
- `WasKeyPressed()`
- `WasKeyReleased()`
- `IsMouseButtonDown()`
- `WasMouseButtonPressed()`
- `WasMouseButtonReleased()`
- `GetMouseX()`
- `GetMouseY()`
- `GetMouseDeltaX()`
- `GetMouseDeltaY()`
- `GetMouseWheelDelta()`

Rendering:

- `SetFrameCallback()`
- `ClearFrame()`
- `PutFramePixel()`
- `PresentFrame()`
- `GetFrameWidth()`
- `GetFrameHeight()`

Settings:

- `GetSettingsPath()`
- `GetFpsLimit()`
- `SetFpsLimit()`

Memory:

- `NewObject<TestData>()`
- `DeleteObject()`
- `NewArray<int>()`
- `DeleteArray()`
- `GetCurrentMemoryBytes()`
- `GetPeakMemoryBytes()`
- `GetMemoryAllocationCount()`
- `GetMemoryFreeCount()`

Diagnostics and timing:

- `GetDeltaTime()`
- `GetDiagnosticsUptime()`
- `GetDiagnosticsLoopCount()`
- `GetTotalTime()`

## Overlay Text Path

The current in-window text path is:

1. `EngineCore::ServiceScheduledWork()`
2. `PlatformModule::SetOverlayText(...)`
3. `PlatformWindow::SetOverlayText(...)`
4. `InvalidateRect(...)`
5. `WM_PAINT`
6. `PlatformWindow::DrawOverlayText(HDC dc)`
7. `DrawTextW(...)`

This is a temporary debug-style overlay path built on top of Win32 and GDI.
It is useful before a dedicated render module exists.

## Render Path

The current render path is:

1. `App` toggles and drives a pixel test through the per-frame callback
2. `EngineCore` forwards rendering calls to `RenderModule`
3. `RenderModule` resizes its software backbuffer to the current client area
4. pixels are written into a CPU-side `std::vector<std::uint32_t>`
5. `Present()` blits the backbuffer to the window with `StretchDIBits`

The current renderer is a small software rendering foundation, not a GPU renderer yet.

## Input Path

The current keyboard and mouse input path is:

1. `PlatformWindow` captures `WM_KEYDOWN` and `WM_KEYUP`
2. `PlatformWindow` also captures mouse move, mouse button, and wheel messages
3. `PlatformModule` exposes those events
4. `InputModule::Update()` reads platform events each frame
5. `InputModule` stores:
   - current key state
   - pressed-this-frame state
   - released-this-frame state
   - mouse position
   - mouse delta
   - wheel delta
   - mouse button state
6. `EngineCore` exposes input queries
7. engine or app code asks questions like `WasKeyPressed(...)` or `GetMouseX()`

Escape shutdown now uses the input query path instead of direct raw key event handling.

## File Path

The current file path is:

1. `App` or engine code calls an `EngineCore` file wrapper
2. `EngineCore` forwards the request to `FileModule`
3. `FileModule` uses `std::filesystem` and file streams
4. results are returned as success or failure without crashing the engine

The current file feature test writes and reads files under `./runtime_test/`.

## Settings Path

The current settings path is:

1. `EngineCore::Initialize()` loads `config/engine.ini`
2. if the file does not exist, Core writes a default settings file
3. Core currently applies `fps_limit`
4. `EndFrame()` uses the loaded FPS limit to pace the frame

The first shipped setting is:

```ini
[core]
fps_limit=60
```

## GPU Compute Path

The current GPU compute path is:

1. `App` or engine code builds a `GpuTaskDesc`
2. `EngineCore` forwards submission to `GpuComputeModule`
3. `GpuComputeModule` validates the descriptor
4. the current backend reports `Stub`
5. submission fails cleanly when GPU compute is unavailable

This is a foundation layer only. It is intentionally not pretending to be a finished GPU compute backend yet.

## Build Command

The current build command you have been using is:

```powershell
Remove-Item -Recurse -Force build
cmake -S . -B build -G Ninja
cmake --build build
```

`build/` is now ignored by git, so generated files should not be part of future pushes.

## Current Gaps

The engine is functional, but still early. Current obvious next steps are:

- leak reporting in `MemoryModule`
- class-by-class memory stats reporting
- more engine settings beyond `fps_limit`
- a render module
- a real GPU compute backend behind `GpuComputeModule`
- richer app-side feature scenarios
- clearer separation between app-facing APIs and internal-only module APIs
