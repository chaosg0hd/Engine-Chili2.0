# Engine API Map

This is the practical map of the engine-facing APIs we use today.

Goal:

- show the main call surfaces without making you dig through headers first
- give a quick "when do I use this?" note for each area
- include tiny usage snippets for the common paths

Source of truth:

- [engine_core.hpp](C:/Users/Neilwinn%20Pineda/Documents/Engine-Chili2.0/src/core/engine_core.hpp)
- [sandbox_app.hpp](C:/Users/Neilwinn%20Pineda/Documents/Engine-Chili2.0/apps/sandbox/src/sandbox_app.hpp)
- [app_capabilities.hpp](C:/Users/Neilwinn%20Pineda/Documents/Engine-Chili2.0/src/app/app_capabilities.hpp)

## 1. App Entry

Files:

- [main.cpp](C:/Users/Neilwinn%20Pineda/Documents/Engine-Chili2.0/apps/sandbox/src/main.cpp)
- [sandbox_app.hpp](C:/Users/Neilwinn%20Pineda/Documents/Engine-Chili2.0/apps/sandbox/src/sandbox_app.hpp)

Public calls:

- `SandboxApp::Run()`

Use it when:

- you want to boot the active sandbox harness

Example:

```cpp
SandboxApp app;
return app.Run() ? 0 : 1;
```

## 2. Engine Lifecycle

File:

- [engine_core.hpp](C:/Users/Neilwinn%20Pineda/Documents/Engine-Chili2.0/src/core/engine_core.hpp)

Main calls:

- `Initialize()`
- `Run()`
- `Tick()`
- `Shutdown()`
- `RequestShutdown()`
- `SetFrameCallback(FrameCallback callback)`

Use them when:

- `Initialize()` boots modules and creates the runtime
- `Run()` enters the normal loop
- `Tick()` is for host-driven integration where something else owns the outer loop
- `Shutdown()` tears the engine down cleanly
- `SetFrameCallback(...)` lets an app provide its per-frame work

Example:

```cpp
EngineCore core;
if (!core.Initialize())
{
    return false;
}

core.SetFrameCallback(
    [](AppCapabilities& capabilities)
    {
        capabilities.window->SetOverlayText(L"hello");
    });

const bool ok = core.Run();
core.Shutdown();
```

## 3. Logging And Overlay

Main calls:

- `LogInfo(...)`
- `LogWarn(...)`
- `LogError(...)`
- `GetLogFilePath()`
- `IsFileLoggingAvailable()`
- `BuildDebugViewText()`
- `ShowDebugView()`
- `ClearWindowOverlayText()`
- `IsWindowOverlayEnabled()`
- `SetWindowOverlayEnabled(...)`
- `AppendWindowOverlayText(...)`
- `SetWindowOverlayText(...)`

Use them when:

- you need runtime diagnostics without reaching into `LoggerModule` directly
- you want quick in-window text for sandbox/debug workflows

Example:

```cpp
core.LogInfo("Sandbox ready.");
core.SetWindowOverlayText(L"RUNTIME SANDBOX\nrunning");
```

## 4. Frame Submission And Legacy Debug Drawing

Main calls:

- `SubmitRenderFrame(const FramePrototype& frame)`
- `ClearFrame(...)`
- `PutFramePixel(...)`
- `DrawFrameLine(...)`
- `DrawFrameRect(...)`
- `FillFrameRect(...)`
- `DrawFrameGrid(...)`
- `DrawFrameCrosshair(...)`
- `PresentFrame()`
- `GetFrameWidth()`
- `GetFrameHeight()`
- `GetFrameAspectRatio()`
- `GetRenderSubmittedItemCount()`
- `GetRenderLegacyCompatibilityCommandCount()`

Use them when:

- `SubmitRenderFrame(...)` is the real modern app-facing render path
- the `DrawFrame*` helpers are compatibility/debug helpers

Example:

```cpp
FramePrototype frame;
// fill frame.passes/views/items here
core.SubmitRenderFrame(frame);
```

## 5. Settings And Frame Pacing

Main calls:

- `LoadSettings()`
- `LoadSettings(path)`
- `SaveSettings()`
- `SaveSettings(path)`
- `ResetSettingsToDefaults()`
- `GetSettingsPath()`
- `GetFpsLimit()`
- `SetFpsLimit(...)`
- `SetTargetFramesPerSecond(...)`
- `GetTargetFramesPerSecond()`
- `GetTargetFrameTime()`
- `GetLastFrameDuration()`
- `GetLastFrameWorkDuration()`
- `GetLastFrameWaitDuration()`
- `GetLastFrameLateness()`
- `GetMaxFrameLateness()`
- `GetLateFrameCount()`
- `GetLastFrameSleepCount()`
- `GetLastFrameYieldCount()`
- `GetAdaptiveSleepMargin()`
- `GetAdaptiveYieldMargin()`
- `IsBehindSchedule()`

Use them when:

- you want runtime pacing information or basic settings persistence

## 6. File API

Main calls:

- `FileExists(...)`
- `DirectoryExists(...)`
- `CreateDirectory(...)`
- `WriteTextFile(...)`
- `ReadTextFile(...)`
- `WriteJsonFile(...)`
- `ReadJsonFile(...)`
- `WriteBinaryFile(...)`
- `ReadBinaryFile(...)`
- `DeleteFile(...)`
- `GetFileSize(...)`
- `GetWorkingDirectory()`
- `ListDirectory(...)`
- `ListFiles(...)`
- `ListDirectories(...)`
- `GetAbsolutePath(...)`
- `NormalizePath(...)`
- `CopyFile(...)`
- `MoveFile(...)`

Use them when:

- you want app-side file IO without reaching directly into `FileModule`

Example:

```cpp
core.CreateDirectory("logs");
core.WriteTextFile("logs/example.txt", "hello");
```

## 7. Resources

Main calls:

- `RequestResource(...)`
- `UnloadResource(...)`
- `GetResourceState(...)`
- `GetResourceKind(...)`
- `GetResourceAssetId(...)`
- `GetResourceResolvedPath(...)`
- `GetResourceLastError(...)`
- `GetResourceSourceByteSize(...)`
- `GetResourceSourceText(...)`
- `GetResourceGpuHandle(...)`
- `GetResourceUploadedByteSize(...)`
- `IsResourceReady(...)`
- `GetResourceCountByState(...)`
- `GetTrackedResourceCount()`

Use them when:

- you need engine-owned async resource tracking

Example:

```cpp
const ResourceHandle handle = core.RequestResource("assets/test.prototype.json", ResourceKind::Text);
if (core.IsResourceReady(handle))
{
    const std::string text = core.GetResourceSourceText(handle);
}
```

## 8. Prototype Registry

Main calls:

- `RegisterPrototype(...)`
- `LoadPrototypeAsset(...)`
- `UnregisterPrototype(...)`
- `GetPrototype(...)`
- `HasPrototype(...)`
- `CreatePrototypeInstance(...)`
- `DestroyPrototypeInstance(...)`
- `GetPrototypeInstanceRuntimeData(...)`
- `GetPrototypeInstancePrototypeId(...)`
- `GetRegisteredPrototypeCount()`
- `GetLivePrototypeInstanceCount()`

Use them when:

- you want engine-owned prototype registration and live instance tracking

## 9. GPU And Compute

Main calls:

- `IsGpuComputeAvailable()`
- `SubmitGpuTask(...)`
- `WaitForGpuIdle()`
- `GetGpuBackendName()`
- `SupportsGpuBuffers()`
- `SupportsComputeDispatch()`
- `GetGpuCapabilitySummary()`
- `GetGpuTrackedResourceCount()`
- `GetGpuTrackedResourceBytes()`

Use them when:

- you need compute capability checks or generic GPU diagnostics

## 10. Window And Input

Main calls:

- `GetDeltaTime()`
- `IsWindowOpen()`
- `IsWindowActive()`
- `GetWindowTitle()`
- `IsWindowMaximized()`
- `IsWindowMinimized()`
- `GetWindowHandle()`
- `GetWindowWidth()`
- `GetWindowHeight()`
- `GetWindowAspectRatio()`
- `SetWindowTitle(...)`
- `MaximizeWindow()`
- `RestoreWindow()`
- `MinimizeWindow()`
- `SetWindowSize(...)`
- `SetCursorVisible(...)`
- `IsCursorVisible()`
- `SetCursorLocked(...)`
- `IsCursorLocked()`
- `IsAnyKeyPressed()`
- `IsKeyDown(...)`
- `WasKeyPressed(...)`
- `WasKeyReleased(...)`
- `IsMouseButtonDown(...)`
- `WasMouseButtonPressed(...)`
- `WasMouseButtonReleased(...)`
- `GetMouseX()`
- `GetMouseY()`
- `GetMouseDeltaX()`
- `GetMouseDeltaY()`
- `GetMouseWheelDelta()`
- `GetMouseNormalizedX()`
- `GetMouseNormalizedY()`

Use them when:

- you need the app-facing input/window surface from `EngineCore`

Example:

```cpp
if (core.WasKeyPressed('R'))
{
    core.LogInfo("Reset requested.");
}
```

## 11. Jobs

Main calls:

- `SubmitJob(...)`
- `WaitForAllJobs()`
- `GetJobWorkerCount()`
- `GetIdleJobWorkerCount()`
- `GetQueuedJobCount()`
- `GetPeakQueuedJobCount()`
- `GetActiveJobCount()`

Use them when:

- you want background work from app-side code without depending on `JobModule`

Example:

```cpp
core.SubmitJob(
    []()
    {
        // background work
    });
```

## 12. Memory

Main calls:

- `AllocateMemory(...)`
- `FreeMemory(...)`
- `NewObject<T>(...)`
- `DeleteObject<T>(...)`
- `NewArray<T>(...)`
- `DeleteArray<T>(...)`
- `GetCurrentMemoryBytes()`
- `GetPeakMemoryBytes()`
- `GetMemoryAllocationCount()`
- `GetMemoryFreeCount()`
- `GetMemoryStats()`
- `GetMemoryBytesByClass(...)`
- `GetPeakMemoryBytesByClass(...)`
- `BuildMemoryReport()`

Use them when:

- you want engine-tracked allocations

## 13. Sound

Main calls:

- `IsSoundAvailable()`
- `RegisterSound(...)`
- `SetMasterVolume(...)`
- `GetMasterVolume()`
- `SetSoundMuted(...)`
- `IsSoundMuted()`
- `StopAllSounds()`
- `PlayOneShotSound(...)`
- `PlayLoopSound(...)`
- `StopSoundById(...)`

Use them when:

- you need the engine-owned audio path for short registered clips

Example:

```cpp
SoundClipDesc clip;
clip.id = "engine.test_tone";
clip.sourcePath = "library/audio/engine_test_tone.wav";

core.RegisterSound(clip);
core.PlayOneShotSound("engine.test_tone");
```

## 14. Diagnostics And Runtime Counters

Main calls:

- `GetTotalTime()`
- `GetDiagnosticsUptime()`
- `GetDiagnosticsLoopCount()`
- `HasPendingJobs()`
- `IsJobSystemIdle()`
- `GetSubmittedJobCount()`
- `GetCompletedJobCount()`
- `GetFailedJobCount()`

Use them when:

- you want lightweight runtime health/counter info

## 15. Runtime Policy

Main calls through `AppCapabilities.runtime`:

- `GetPressureState()`
- `GetLogicPressureState()`
- `GetPresentationPressureState()`
- `IsDegradedMode()`
- `IsEmergencyMode()`
- `GetLogicThrottleLevel()`
- `GetPresentationThrottleLevel()`
- `GetPublishedPresentationSnapshotVersion()`

Use them when:

- app-side code needs to obey engine-owned runtime policy without reaching into engine internals

Example:

```cpp
if (capabilities.runtime->IsEmergencyMode())
{
    // stop optional work
}
```

## 16. Web Dialog API

Main calls:

- `CreateWebDialog(...)`
- `DestroyWebDialog(...)`
- `DestroyAllWebDialogs()`
- `SetWebDialogContentPath(...)`
- `SetWebDialogDockMode(...)`
- `SetWebDialogBounds(...)`
- `SetWebDialogVisible(...)`
- `IsWebDialogReady(...)`
- `IsWebDialogOpen(...)`
- `GetWebDialogBounds(...)`

Use them when:

- you want engine-owned WebView2 tools/dialog surfaces

## 17. Native Button API

Main calls:

- `CreateNativeButton(...)`
- `DestroyNativeButton(...)`
- `DestroyAllNativeButtons()`
- `SetNativeButtonBounds(...)`
- `SetNativeButtonText(...)`
- `SetNativeButtonVisible(...)`
- `ConsumeNativeButtonPressed(...)`
- `IsNativeButtonOpen(...)`

Use them when:

- you want simple engine-owned native widgets

## 18. Runtime Sandbox Helpers

These are the main helpers around the current runtime-stability harness and the retained scene-preset lane.

Files:

- [runtime_stress_frame_compiler.hpp](C:/Users/Neilwinn%20Pineda/Documents/Engine-Chili2.0/src/prototypes/compiler/runtime_stress_frame_compiler.hpp)
- [runtime_stress_policy.hpp](C:/Users/Neilwinn%20Pineda/Documents/Engine-Chili2.0/src/prototypes/systems/runtime_stress_policy.hpp)
- [sandbox_scene_presets.hpp](C:/Users/Neilwinn%20Pineda/Documents/Engine-Chili2.0/src/prototypes/compiler/sandbox_scene_presets.hpp)

### `RuntimeStressFrameCompiler`

Use it when:

- you want reusable screen-patch presentation stress without putting frame-building logic in the app harness

### `ClampRuntimeStressLoadForPressure(...)`
### `ComputeRuntimeStressMemoryBlockTarget(...)`

Use them when:

- you want app-side runtime stress to obey engine pressure state consistently

### `SandboxScenePresetOptionsPrototype`

Useful fields:

- `totalTime`
- `rotationPaused`
- `cameraOrbitEnabled`
- `cameraOverrideEnabled`
- `cameraControlCamera`
- `primaryLight`
- material pointers for the retained lighting-lab scene preset

Use it when:

- you want to build the retained scene-preset lane without hardcoding scene assembly

## 19. Studio Project Workflow

These APIs live in `apps/studio/src/studio/` and are Studio-layer only. They do not belong in `EngineCore`, renderer modules, sandbox apps, or game runtime modules.

Files:

- [file_proxy.hpp](C:/Users/Neilwinn%20Pineda/Documents/Engine-Chili2.0/apps/studio/src/studio/file_proxy.hpp)
- [studio_project_system.hpp](C:/Users/Neilwinn%20Pineda/Documents/Engine-Chili2.0/apps/studio/src/studio/studio_project_system.hpp)
- [studio_template_system.hpp](C:/Users/Neilwinn%20Pineda/Documents/Engine-Chili2.0/apps/studio/src/studio/studio_template_system.hpp)
- [file_management_dialog.hpp](C:/Users/Neilwinn%20Pineda/Documents/Engine-Chili2.0/apps/studio/src/studio/file_management_dialog.hpp)
- [new_project_dialog.hpp](C:/Users/Neilwinn%20Pineda/Documents/Engine-Chili2.0/apps/studio/src/studio/new_project_dialog.hpp)
- [project_explorer_panel.hpp](C:/Users/Neilwinn%20Pineda/Documents/Engine-Chili2.0/apps/studio/src/studio/project_explorer_panel.hpp)

### `FileProxy`

Main calls:

- `Resolve(...)`
- `Exists(...)`
- `CreateDirectory(...)`
- `WriteText(...)`
- `ReadText(...)`
- `ListDirectory(...)`
- `IsDirectory(...)`

Use it when:

- Studio needs disk or path access inside the logical `User/` workspace
- Studio UI/backend code needs logical paths instead of absolute OS paths

### `StudioProjectSystem`

Main calls:

- `CreateProject(...)`
- `OpenProject(...)`
- `SaveProject(...)`
- `GetCurrentProject()`
- `GetCurrentProjectRoot()`
- `MakeProjectId(...)`

Use it when:

- the Studio File dialog creates, opens, or saves user projects
- the Project Explorer needs the currently opened project root

Current workspace shape:

```text
User/
  <project_id>/
    Project/
      project.enginegame
      CMakeLists.txt
      src/
      assets/
      scenes/
      config/
    Build/
    Cache/
    Logs/
```

### `StudioTemplateSystem`

Main call:

- `ApplyTemplate(templateName, targetProjectPath, projectName, projectId)`

Current built-in template:

- `Arcade2D`

Generated starter files:

- `src/main.cpp`
- `src/<project_id>.hpp`
- `src/<project_id>.cpp`
- `scenes/main.scene`
- `config/game.config`
- `CMakeLists.txt`

### Studio Web Surfaces

Current Studio HTML surfaces:

- top bar: `apps/studio/coretools/runtime/top-bar/top-bar.html`
- left bar: `apps/studio/coretools/runtime/left-bar/left-bar.html`
- right bar: `apps/studio/coretools/runtime/right-bar/right-bar.html`
- bottom bar: `apps/studio/coretools/runtime/bottom-bar/bottom-bar.html`
- File dialog: `apps/studio/coretools/runtime/dialogs/file-dialog.html`
- New Project dialog: `apps/studio/coretools/runtime/dialogs/new-project-dialog.html`

Studio host routes:

- `/studio/open-file-management`
- `/studio/open-new-project`
- `/studio/create-project`
- `/studio/open-project`
- `/studio/save-project`
- `/studio/project-explorer/tree`
- `/studio/project-explorer/select`

### `ProjectExplorerPanel`

Main calls:

- `Open(...)`
- `BuildTreeJson(...)`
- `SelectFile(...)`
- `GetSelectedFileLogicalPath()`

Use it when:

- Studio needs a right-docked project file tree
- a file click should update Studio-owned selected-file state

Selection rules:

- selected paths must stay inside the current project root
- directory clicks expand/collapse in HTML
- file clicks update `ProjectExplorerPanel` selected path

### Studio Viewport Controls

Bindings registered in `StudioRuntimeHost::ConfigureInputContexts()`:

| Action | Binding |
|--------|---------|
| Orbit | MMB drag |
| Pan | Shift + MMB drag |
| Zoom | Scroll wheel |
| Select entity | LMB click |
| Multi-select | Shift + LMB click |
| Focus selection | F |

## 20. What To Reach For First

If you are building app-side code:

- start with `EngineCore`
- use `SubmitRenderFrame(...)` for rendering
- use `SetFrameCallback(...)` to drive frame logic
- use `AppCapabilities.runtime` to obey engine-owned pressure policy
- use `AppCapabilities.sound` for app-side sound control

If you are working on the active sandbox:

- start with `SandboxApp`
- treat `RuntimeStressFrameCompiler` and `runtime_stress_policy.hpp` as the reusable helpers
- treat the sandbox as a harness, not as an engine-side logic owner

If you are working on the retained scene-preset lane:

- start with `SandboxScenePresetOptionsPrototype`
- treat the prototype scene data as passive input
- keep renderer execution logic out of app-side code

If you are working on Studio project management:

- start with `StudioProjectSystem`
- use `FileProxy` for all filesystem operations
- use Studio Web Dialog wrappers for UI surfaces
- keep generated user projects out of the engine source tree

## 21. Follow-Up Docs

- [Docs README](../README.md)
- [Engine Architecture TODO](./TODO.md)
- [Architecture Rules](./ARCHI_RULES)
