# Engine Core Documentation

## Overview

`engine_core` is the native runtime layer for Engine-Chili2.0.
It boots the engine modules, owns the frame lifecycle, runs the Win32 platform window,
tracks timing and diagnostics, and exposes the app-facing API through `EngineCore`.

The current sandbox execution path is:

1. `apps/sandbox/src/main.cpp` creates `SandboxApp`
2. `SandboxApp::Run()` creates `EngineCore`
3. `EngineCore::Initialize()` creates and initializes engine modules
4. `SandboxApp` builds a prototype-driven frame and submits it through `EngineCore`
5. `EngineCore::Run()` enters the runtime loop
6. `EngineCore::Shutdown()` shuts modules down in reverse order

## Structure

### Entry Layer

- `apps/sandbox/src/main.cpp`
  - Creates `SandboxApp`
  - Calls `SandboxApp::Run()`
- `apps/sandbox/src/sandbox_app.hpp`
- `apps/sandbox/src/sandbox_app.cpp`
  - Holds the current sandbox renderer bring-up scene
  - Builds prototype-driven room-style 3D frame content
  - Enters the engine runtime loop
- `apps/sandbox/src/sandbox_builtin_meshes.hpp`
  - Temporary sandbox-owned test geometry definitions used for DX11 bring-up
- `src/main.cpp`
  - Creates the generic root `App`
  - Calls `App::Run()`
- `src/app/app.hpp`
- `src/app/app.cpp`
  - Minimal engine runner that keeps a basic debug overlay active

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
5. `GpuModule`
6. `RenderModule`
7. `ResourceModule`
8. `InputModule`
9. `JobModule`
10. `MemoryModule`
11. `FileModule`
12. `GpuComputeModule`
13. `WebViewModule`
14. `NativeUiModule`

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
- GPU-backed frame presentation through `GpuModule`
- render orchestration and prototype-to-render translation through `RenderModule`
- engine-facing resource tracking and async file-to-GPU upload scaffolding through `ResourceModule`
- in-window GDI overlay text
- background job dispatch through `JobModule`
- keyboard and mouse state queries through `InputModule`
- raw and typed memory allocation through `MemoryModule`
- file and directory utilities through `FileModule`
- GPU compute capability queries through `GpuComputeModule`
- engine-owned WebView2 dialogs through `WebViewModule`
- engine-owned native UI widgets through `NativeUiModule`

## Public API Inventory

### `SandboxApp`

File: `apps/sandbox/src/sandbox_app.hpp`

- `bool Run()`

Private helpers used by the current sandbox harness:

- `void UpdateFrame(EngineCore& core)`
- `void UpdateInput(EngineCore& core)`
- `FramePrototype BuildDemoFrame() const`
- `void AppendOverlayLines(EngineCore& core) const`

### `App`

File: `src/app/app.hpp`

- `bool Run()`
- `void UpdateFrame(EngineCore& core)`

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
- `std::string GetLogFilePath() const`
- `bool IsFileLoggingAvailable() const`
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
- `double GetLastFrameDuration() const`
- `double GetLastFrameLateness() const`
- `double GetMaxFrameLateness() const`
- `unsigned long long GetLateFrameCount() const`
- `bool IsBehindSchedule() const`

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
- `std::size_t GetRenderSubmittedItemCount() const`
- `std::size_t GetRenderLegacyCompatibilityCommandCount() const`

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
- `std::size_t GetGpuTrackedResourceCount() const`
- `std::size_t GetGpuTrackedResourceBytes() const`

Web dialogs:

- `using WebDialogHandle = std::uint32_t`
- `WebDialogHandle CreateWebDialog(const WebDialogDesc& desc)`
- `bool DestroyWebDialog(WebDialogHandle handle)`
- `void DestroyAllWebDialogs()`
- `bool SetWebDialogContentPath(WebDialogHandle handle, const std::string& contentPath)`
- `bool SetWebDialogDockMode(WebDialogHandle handle, WebDialogDockMode dockMode, int dockSize)`
- `bool SetWebDialogBounds(WebDialogHandle handle, const WebDialogRect& rect)`
- `bool SetWebDialogVisible(WebDialogHandle handle, bool visible)`
- `bool IsWebDialogReady(WebDialogHandle handle) const`
- `bool IsWebDialogOpen(WebDialogHandle handle) const`
- `WebDialogRect GetWebDialogBounds(WebDialogHandle handle) const`

Native UI:

- `using NativeButtonHandle = std::uint32_t`
- `NativeButtonHandle CreateNativeButton(const NativeButtonDesc& desc)`
- `bool DestroyNativeButton(NativeButtonHandle handle)`
- `void DestroyAllNativeButtons()`
- `bool SetNativeButtonBounds(NativeButtonHandle handle, const NativeControlRect& rect)`
- `bool SetNativeButtonText(NativeButtonHandle handle, const std::wstring& text)`
- `bool SetNativeButtonVisible(NativeButtonHandle handle, bool visible)`
- `bool ConsumeNativeButtonPressed(NativeButtonHandle handle)`
- `bool IsNativeButtonOpen(NativeButtonHandle handle) const`

Input:

- `double GetDeltaTime() const`
- `bool IsWindowOpen() const`
- `bool IsWindowActive() const`
- `std::wstring GetWindowTitle() const`
- `bool IsWindowMaximized() const`
- `bool IsWindowMinimized() const`
- `HWND GetWindowHandle() const`
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
- `unsigned int GetIdleJobWorkerCount() const`
- `std::size_t GetQueuedJobCount() const`
- `std::size_t GetPeakQueuedJobCount() const`
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
- `std::size_t GetPeakMemoryBytesByClass(MemoryClass memoryClass) const`
- `std::string BuildMemoryReport() const`

Resources:

- `ResourceHandle RequestResource(const std::string& assetId, ResourceKind kind)`
- `bool UnloadResource(ResourceHandle handle)`
- `ResourceState GetResourceState(ResourceHandle handle) const`
- `ResourceKind GetResourceKind(ResourceHandle handle) const`
- `std::string GetResourceAssetId(ResourceHandle handle) const`
- `std::string GetResourceResolvedPath(ResourceHandle handle) const`
- `std::string GetResourceLastError(ResourceHandle handle) const`
- `std::size_t GetResourceSourceByteSize(ResourceHandle handle) const`
- `GpuResourceHandle GetResourceGpuHandle(ResourceHandle handle) const`
- `std::size_t GetResourceUploadedByteSize(ResourceHandle handle) const`
- `bool IsResourceReady(ResourceHandle handle) const`
- `std::size_t GetResourceCountByState(ResourceState state) const`
- `std::size_t GetTrackedResourceCount() const`

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
- `gpu/gpu_module.hpp`
- `gpu/igpu_service.hpp`
- `memory/memory_module.hpp`
- `memory/imemory_service.hpp`
- `memory/memory_types.hpp`
- `platform/iplatform_service.hpp`
- `render/irender_service.hpp`
- `resources/iresource_service.hpp`
- `resources/resource_types.hpp`
- `resources/resource_module.hpp`
- `webview/webview_module.hpp`
- `webview/web_dialog_host.hpp`
- `native_ui/native_ui_module.hpp`

Prototype data contracts now live under `src/prototypes/`:

- `presentation/frame.hpp`
- `presentation/pass.hpp`
- `presentation/view.hpp`
- `presentation/item.hpp`
- `entity/object.hpp`
- `entity/mesh.hpp`
- `entity/material.hpp`
- `entity/camera.hpp`
- `math/math.hpp`

## Data Paths

### Overlay Text Path

1. `SandboxApp` or `App` calls `EngineCore::ShowDebugView()` or `SetWindowOverlayText(...)`
2. `EngineCore` stores or builds overlay text
3. `EngineCore::ServiceScheduledWork()` forwards it to platform
4. `PlatformModule::SetOverlayText(...)`
5. `PlatformWindow::SetOverlayText(...)`
6. `InvalidateRect(...)`
7. `WM_PAINT`
8. `PlatformWindow::DrawOverlayText(HDC dc)`
9. `DrawTextW(...)`

### Render Path

1. `SandboxApp` or `App` drives rendering through the per-frame callback
2. `EngineCore` forwards rendering calls to `RenderModule`
3. `RenderModule` accepts `FramePrototype` requests at the module boundary
4. `RenderModule` translates prototype data into render-owned `RenderFrameData`
5. `RenderModule::Update(...)` submits that render-owned frame state through `IGpuService`
6. `GpuModule` owns backend resize, viewport, generic GPU resources, and presentation state
7. The active backend begins the frame, renders the submitted frame data, and presents the result

Important current limitation:

- the immediate-mode pixel helpers such as `ClearFrame`, `PutFramePixel`, and `PresentFrame` still exist as legacy compatibility paths, but the active DX11 validation path is the prototype-driven frame submission flow

### Prototype Families

Current prototype families:

- `src/prototypes/presentation/`
  - `FramePrototype`
  - `PassPrototype`
  - `ViewPrototype`
  - `ItemPrototype`
- `src/prototypes/entity/`
  - `ObjectPrototype`
  - `MeshPrototype`
  - `MaterialPrototype`
  - `CameraPrototype`
- `src/prototypes/math/`
  - vector, matrix, transform, and shared math helpers

Current ownership rule:

- modules may consume prototype types at their public boundaries
- modules should translate prototype requests into module-private execution state
- `RenderModule` is the active example of that rule through `FramePrototype -> RenderFrameData`

### Input Path

1. `PlatformWindow` captures Win32 keyboard and mouse messages
2. `PlatformModule` exposes those events
3. `InputModule::Update()` reads platform events each frame
4. `InputModule` stores per-frame state
5. `EngineCore` exposes input queries to apps and hosts

### File Path

1. App or engine code calls an `EngineCore` file wrapper
2. `EngineCore` forwards the request through `IFileService`
3. `FileModule` implements that service with `std::filesystem` and file streams
4. Results are returned as success or failure

### Resource Path

1. App or engine code calls a resource wrapper on `EngineCore`
2. `EngineCore` forwards the request through `IResourceService`
3. `ResourceModule` creates or looks up a tracked resource record
4. `ResourceModule` uses `IJobService` and `IFileService` to read source bytes asynchronously
5. `ResourceModule` asks `IGpuService` to create an uploaded GPU resource handle
6. Resource state advances through `Queued`, `Loading`, `Processing`, `Uploading`, then `Ready` or `Failed`

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

### Web Dialog Path

1. App or host code builds a `WebDialogDesc`
2. `EngineCore::CreateWebDialog(...)` forwards it to `WebViewModule`
3. `WebViewModule` creates a `WebDialogHost`
4. `WebDialogHost` creates either a docked child host window or a floating top-level host window
5. `WebView2` loads the requested local content path
6. `WebViewModule::Update()` keeps docked dialogs aligned with the engine window client area

## Build Notes

Run instructions:

```powershell
Remove-Item -Recurse -Force build -ErrorAction SilentlyContinue
cmake -S . -B build -G Ninja
cmake --build build
```

Sanitizer build:

```powershell
Remove-Item -Recurse -Force build\sanitize -ErrorAction SilentlyContinue
cmake -S . -B build\sanitize -G Ninja -DENABLE_SANITIZERS=ON
cmake --build build\sanitize
```

CI automation:

- `.github/workflows/windows-build.yml`

## Current Gaps

- leak reporting in `MemoryModule`
- more engine settings beyond `fps_limit`
- a real GPU compute backend behind `GpuComputeModule`
- deeper dialog behaviors such as drag docking, tab stacks, and engine-to-web messaging
- typed asset decode/import beyond the current raw-binary resource scaffolding
- clearer separation between renderer-private resources and general GPU-owned resources
- a proper long-term home for reusable built-in geometry beyond the current sandbox-owned temporary mesh definitions
- richer app-side feature scenarios beyond the current sandbox harness

## Module Ownership Model

The intended top-level ownership model for the engine is:

- `PlatformModule`: owns the OS-facing window, native handles, message pump integration, and render surface state
- `GpuModule`: owns the graphics backend, device objects, swapchain or presentation path, and generic GPU resource lifetime
- `RenderModule`: owns render orchestration, frame flow, render queues, pass ordering, and renderer-private frame resources
- `ResourceModule`: owns engine-facing resource identity, handles, dependency tracking, and load-state progression
- `JobModule`: owns worker execution, background dispatch, and scheduled async work
- `MemoryModule`: owns allocation policy, memory categories, tracking, diagnostics, and budget visibility

One-sentence version:

Platform provides surfaces, GPU owns device resources, Renderer draws frames, Resources own asset state, Jobs perform work, Memory defines allocation policy.

## Module Dependency Rules

The intended high-level dependency direction is:

```text
App / Scene
    |
ResourceModule
    |
RenderModule
    |
GpuModule
    |
PlatformModule
```

Supporting dependency rules:

- `ResourceModule -> JobModule`
- `RenderModule -> JobModule` only for limited, specific work
- all major systems may consume `MemoryModule`
- `RenderModule` may consume `ResourceModule` outputs, but must not own import logic
- `GpuModule` must not depend on `RenderModule`

This should be treated as the intended architectural contract for future module work, even where the current codebase has not reached that shape yet.

## Ownership Rules

The intended ownership and lifetime rules are:

- only the owning module controls lifetime state
- jobs do work but do not become long-term owners
- submission is not ownership
- GPU handles must not leak as raw backend objects outside the GPU layer
- renderer-private resources stay private

Applied examples:

- only `ResourceModule` should change asset or resource load-state progression
- only `GpuModule` should control generic GPU object lifetime and safe release policy
- only `RenderModule` should control render-frame orchestration state and renderer-private frame resources
- a background job may decode or transform data, but that job should not become the owner of the resulting resource
- a texture or buffer being used by rendering does not mean rendering owns its global lifetime

These rules are meant to keep ownership boundaries explicit before deeper GPU, rendering, and resource work lands in code.

## Render Surface Contract

The intended handoff from `PlatformModule` to `GpuModule` is a render-surface description rather than direct GPU ownership by the platform layer.

Proposed surface contract:

```cpp
struct RenderSurface
{
    void* nativeHandle = nullptr;
    std::uint32_t width = 0;
    std::uint32_t height = 0;
};
```

Ownership and use:

- `PlatformModule` owns the native window and the OS-facing surface state
- `PlatformModule` provides the current `RenderSurface`
- `GpuModule` consumes that surface description to create or manage presentation resources
- `RenderModule` should not own the OS-facing surface directly

Intent:

- keep the platform layer responsible for windowing
- keep the GPU layer responsible for presentation/device binding
- avoid collapsing window ownership and GPU ownership into the same module

## Architecture Planning

The module-boundary planning work now lives in `docs/engine/TODO.md`.


