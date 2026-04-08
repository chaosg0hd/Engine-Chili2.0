# Engine Architecture TODO

## Historic Marker

This file is the current architecture progress log for the engine.

What this is:

- the place where module-boundary planning progress is recorded
- the working checklist for the next structural engine changes
- the reference point for what the project currently is versus what it is trying to become

Current project state:

- the engine is already modular at a basic runtime level, with platform, render, input, jobs, memory, file, GPU compute, and webview modules in place
- `EngineCore` is still the main integration point and currently carries a lot of system wiring responsibility
- rendering is now split across `GpuModule` and `RenderModule`, but the sandbox still carries older immediate-mode test paths
- a real `GpuModule` boundary now exists, though generic GPU resource behavior is still scaffold-level
- resource ownership is now formalized into a first `ResourceModule`, though it is still using raw-binary loading rather than typed asset pipelines
- the current project is in a transition stage from feature scaffolding into clearer production-oriented ownership boundaries

Target direction:

- platform provides surfaces
- GPU owns device resources
- renderer draws frames
- resources own asset state
- jobs perform work
- memory defines allocation policy

## Sandbox Scan

This section is a code-scan snapshot of what the current sandbox app appears to exercise.

Important note:

- this is a static implementation scan, not a runtime-certified test report
- items marked as working mean "implemented enough to exercise in sandbox code"
- items marked as waiting or weak mean "present, but partial, stubbed, or not yet meaningfully exercised"

### Likely Working In Sandbox

- memory typed allocation path
  - `RunMemoryFeatureTest(...)` allocates typed objects and arrays through `EngineCore`
- memory raw allocation path
  - `RunRawMemoryFeatureTest(...)` allocates and frees a raw debug buffer
- file create/write/read path
  - `RunFileFeatureTest(...)` creates a directory and round-trips text and binary files
- job queue execution path
  - `RunJobFeatureTest(...)` submits background jobs and waits for completion
- resource state and upload path
  - `RunResourceFeatureTest(...)` requests both a valid and a missing resource and validates ready/failed results
- input polling path
  - `RunInputFeatureTest(...)` logs initial input state and the runtime loop reads live input
- debug overlay path
  - `RunTextOverlayMode(...)` uses `ShowDebugView()`
- display-mode switching
  - `Tab` toggles text overlay and pixel-renderer mode
- shutdown path
  - `Escape` requests shutdown and window close requests shutdown

### Present But Weak / Partial

- pixel renderer mode
  - `RunPixelRendererMode(...)` still draws a blinking star field through `ClearFrame`, `PutFramePixel`, and `PresentFrame`
  - current render code has those immediate drawing methods as placeholders, so this mode is not yet trustworthy as a real renderer validation
- GPU module ownership split
  - the graphics backend and first generic uploaded-resource tracking now live under `GpuModule`
  - broader backend-backed GPU services are still scaffold-level rather than deeply exercised
- render scene path
  - `RenderScene` types exist and `RenderModule::SubmitScene(...)` exists
  - but the sandbox app does not appear to drive scene submission yet

### Stubbed / Not Working Well Yet

- GPU compute feature test as a real compute test
  - `RunGpuFeatureTest(...)` still talks to `GpuComputeModule`
  - `GpuComputeModule` remains a stub backend, so this is mostly a capability/stub-consistency check
- renderer-private versus general GPU resources
  - architecture direction is clearer now, but the separation is not yet fully expressed in features
- resource-to-GPU upload handoff
  - `ResourceModule` now hands off binary payloads to `IGpuService` and gets back engine-owned GPU handles
  - this is still scaffold-level upload tracking rather than a richer backend-backed asset pipeline
- sandbox coverage for web dialogs / native UI
  - engine APIs exist, but the sandbox app does not appear to exercise those features directly

### Suggested Manual Sandbox Test Pass

- verify memory feature tests still log allocate/free counters correctly
- verify file feature test still creates and round-trips files under `runtime_test`
- verify job feature test completes all queued jobs
- verify input responds to mouse move, click, and wheel activity during runtime
- verify text overlay mode still shows the debug view
- verify pixel mode toggle still switches modes, even if the renderer path is still placeholder-heavy
- verify the resource-system sandbox test reports ready/failed transitions and uploaded-byte metadata visibly

## TODO

Task:

- establish and document the top-level module ownership model

Progress/Note:

- codebase scan on 2026-04-08 confirmed the current state:
  - `EngineCore` still performs most module wiring manually
  - `RenderModule` still owns the graphics backend path
  - `GpuComputeModule` exists, but it is separate from the render backend stack
  - the intended ownership model is only partially reflected in code today
- implementation progress on 2026-04-08:
  - `PlatformModule` now exposes a `RenderSurface` contract
  - `RenderModule` now consumes `RenderSurface` instead of reaching directly into `PlatformWindow`
  - this is the first landed ownership boundary slice in code
  - `InputModule`, `WebViewModule`, and `NativeUiModule` now self-bind to `context.Platform`
  - `EngineCore` no longer manually injects `PlatformModule` into those modules during startup wiring
  - this reduces central glue and makes module ownership a little less dependent on `EngineCore` hand-stitching
  - `RenderModule` now self-binds to `context.Platform` as well
  - unused `SetPlatformModule(...)` hooks were removed from `RenderModule`, `InputModule`, `WebViewModule`, and `NativeUiModule`
  - module interfaces now better match the current ownership direction instead of exposing leftover manual-injection plumbing
  - a real graphics-facing `GpuModule` now exists in the codebase and is registered by `EngineCore`
  - `EngineCore` now carries `Gpu` and `Resources` references in `EngineContext`
  - a first `ResourceModule` lane now exists in code instead of only in planning
- documented ownership lanes remain:
  - `PlatformModule`
  - `GpuModule`
  - `RenderModule`
  - `ResourceModule`
  - `JobModule`
  - `MemoryModule`
- build verification:
  - unsandboxed `cmake --build build` succeeded after each ownership-boundary change landed
- status:
  - in progress
  - not complete until the current codebase reflects these ownership lanes more broadly than the current platform/gpu/render/resource scaffolding

## TODO

Task:

- improve sandbox/runtime logging so crashes and failed startup paths are visible immediately

Progress/Note:

- immediate reason:
  - the latest sandbox smoke run stayed alive briefly, but the observed behavior still suggests it may be crashing or failing without enough visible feedback
- implementation progress on 2026-04-08:
  - confirmed an existing engine logging API already existed through `EngineCore -> LoggerModule`
  - confirmed that previous logging only wrote to the console and did not persist to a runtime file
  - `LoggerModule` now writes to both the console and `logs/engine_runtime.log`
  - each log line now includes a timestamp and is flushed immediately so startup failures are visible before shutdown or crash
  - `EngineCore` now exposes the runtime log path and whether file logging is available
  - the debug overlay now shows the runtime log path
  - the sandbox app now logs the runtime log destination during startup and emits an explicit startup-failure note that points back to the log file
  - the sandbox app now records per-feature `PASS` / `FAIL` results and logs a startup feature summary
  - text-overlay mode now shows a startup-check summary above the live debug view
- short-term goal:
  - make sandbox feature-test progress and failure states visible without relying on guesswork
- likely next work items:
  - keep adding richer detail to failed summary items instead of only pass/fail labels
  - add explicit runtime summaries for web/native-ui coverage once those features are exercised
- this is now a practical testing blocker, so it should be treated as a near-term priority

## DONE

Task:

- define the dependency direction rules between core modules

Progress/Note:

- finished on 2026-04-08
- the dependency direction rules are now documented in `docs/engine/README.md`
- intended dependency flow is:

```text
App / Scene
    ↓
ResourceModule
    ↓
RenderModule
    ↓
GpuModule
    ↓
PlatformModule
```

- supporting rules already identified:
  - `ResourceModule -> JobModule`
  - `RenderModule -> JobModule` only for limited, specific work
  - all major systems may consume `MemoryModule`
  - `GpuModule` must not depend on `RenderModule`
- this should be treated as a contract, not just a suggestion

## DONE

Task:

- formalize ownership rules for lifetime and responsibility boundaries

Progress/Note:

- finished on 2026-04-08
- the ownership rules are now documented in `docs/engine/README.md`
- core ownership rules are now recorded as the intended contract:
  - only the owning module controls lifetime state
  - jobs do work but do not become long-term owners
  - submission is not ownership
  - GPU handles should not leak as raw backend objects outside the GPU layer
  - renderer-private resources should stay private
- this should become a written contract before deeper GPU/resource work continues

## DONE

Task:

- define the `PlatformModule` to `GpuModule` handoff through a render surface contract

Progress/Note:

- finished on 2026-04-08
- the render-surface handoff is now documented in `docs/engine/README.md`
- proposed surface boundary:

```cpp
struct RenderSurface
{
    void* nativeHandle = nullptr;
    std::uint32_t width = 0;
    std::uint32_t height = 0;
};
```

- intended ownership:
  - `PlatformModule` owns the OS-facing window and surface state
  - `GpuModule` consumes the surface description
- this is the first clean handoff needed for untangling window ownership from GPU ownership

## TODO

Task:

- introduce a real `GpuModule` boundary and move generic GPU ownership into it

Progress/Note:

- implementation progress on 2026-04-08:
  - added `src/modules/gpu/gpu_module.hpp` and `src/modules/gpu/gpu_module.cpp`
  - moved render-backend ownership, viewport tracking, resize handling, and frame presentation into `GpuModule`
  - `EngineCore` now creates `GpuModule` before `RenderModule`
  - graphics backend naming now comes from `GpuModule` instead of the compute stub path
  - `IGpuService` now exposes a first generic uploaded-resource API with engine-owned GPU resource handles
  - `GpuModule` now tracks uploaded resource handles, kinds, and byte sizes instead of only acting as a frame-present path
- target ownership remains:
  - `GpuModule` should own:
  - backend selection
  - device/context/queues
  - swapchain or presentation path
  - generic GPU resources
  - upload path
  - readback path later
  - deferred safe release
  - capability queries
- `DX11` should be treated as the first backend implementation inside `GpuModule`, not as the renderer itself
- this unlocks future compute, readback, texture processing, and non-render GPU work without collapsing everything into rendering
- status:
  - in progress
  - backend ownership is now in `GpuModule`, and a first generic resource-upload surface now exists
  - broader backend-backed GPU resource behavior is still scaffold-level rather than fully renderer-independent

## TODO

Task:

- narrow `RenderModule` so it becomes a renderer client of GPU services rather than the owner of the whole graphics stack

Progress/Note:

- implementation progress on 2026-04-08:
  - `RenderModule` no longer owns backend creation or backend lifetime
  - `RenderModule` now renders through `IGpuService` provided by `EngineContext`
  - `RenderModule` is now mostly carrying scene submission state and clear-color/frame orchestration intent
- `RenderModule` should own:
  - frame flow
  - render queues
  - camera/view setup
  - draw submission translation
  - render pass ordering
  - renderer-private attachments/resources
- `RenderModule` should not own:
  - broad GPU object lifetime policy
  - non-render compute usage
  - asset import or file loading pipelines
- this work depends on the `GpuModule` boundary being clear first
- status:
  - in progress
  - the backend moved out, but renderer-private resource separation and richer render submission are still ahead

## TODO

Task:

- create a dedicated `ResourceModule` for engine-facing asset and resource state

Progress/Note:

- implementation progress on 2026-04-08:
  - added `src/modules/resources/resource_module.hpp` and `src/modules/resources/resource_module.cpp`
  - added resource handles, resource kinds, and resource load-state tracking
  - registered `ResourceModule` in `EngineCore`
  - exposed first resource wrappers through `EngineCore` for request, unload, state query, and tracked-count query
  - `ResourceModule` now uses `JobModule` to advance requested resources asynchronously through queued/loading/processing/ready
  - `ResourceModule` now has file-aware failure handling and can mark resources `Failed` when the requested asset path does not exist
  - `ResourceModule` now uses an `Uploading` state when GPU services are present
  - `ResourceModule` now performs real file-backed binary reads instead of only checking file existence
  - resource records now track resolved path, last error, and source byte size
  - `EngineCore` now exposes resource inspection helpers for kind, asset id, resolved path, last error, and source byte size
  - resource records now also track GPU upload handle and uploaded byte count
  - `ResourceModule` unload now releases its GPU-owned uploaded resource handle through `IGpuService`
  - `IResourceService`, `ResourceModule`, and `EngineCore` now expose resource counts by state
  - the debug view now shows resource totals plus queued/loading/processing/uploading/ready/failed counts
- resource ownership is currently one of the biggest missing lanes
- `ResourceModule` should own:
  - asset/resource registry
  - resource handles
  - dependency tracking
  - load state machine
  - CPU-side parsed asset data
  - requests for GPU upload through `GpuModule`
- suggested state path:

```text
Unloaded -> Queued -> Loading -> Processing -> Uploading -> Ready
```

- this state should belong to `ResourceModule`, not to jobs and not to rendering
- status:
  - in progress
  - the state machine and handle table exist, and a first file-to-GPU upload handoff now exists
  - parsing/decoding are still raw binary scaffolding rather than typed asset pipelines

## TODO

Task:

- define the async handoff between `ResourceModule`, `JobModule`, and `GpuModule`

Progress/Note:

- target texture-loading flow is already clear:
  - scene/material asks `ResourceModule` for a texture
  - `ResourceModule` creates or looks up the resource record
  - `JobModule` performs file read and decode work
  - decoded CPU data returns to `ResourceModule`
  - `ResourceModule` asks `GpuModule` to create/upload the GPU texture
  - `GpuModule` returns a GPU handle
  - `ResourceModule` marks the texture ready
  - `RenderModule` consumes the result
- implementation progress on 2026-04-08:
  - `ResourceModule` now performs a real upload handoff through `IGpuService`
  - the upload step now creates an engine-owned GPU resource handle instead of only toggling the `Uploading` state
  - uploaded GPU handle and byte-count metadata are now queryable from the resource lane
- this flow is a strong reference model for future mesh/material/shader loading too

## TODO

Task:

- clarify renderer-private resources versus general GPU resources

Progress/Note:

- current intended split:
  - `GpuModule` owns generic buffers, textures, samplers, shader objects, views, upload/readback buffers
  - `RenderModule` owns backbuffer views, depth targets, frame attachments, pass-temporary targets, and render-only constant buffers
- implementation progress on 2026-04-08:
  - generic uploaded resource tracking now lives in `GpuModule`
  - `RenderModule` still owns only frame orchestration state and does not store those generic uploaded handles
- this separation helps prevent frame-local render internals from leaking into the engine-wide resource model

## TODO

Task:

- lock the service interface layer for platform, GPU, render, resource, jobs, and memory

Progress/Note:

- proposed interface set:
  - `IPlatformService`
  - `IGpuService`
  - `IRenderService`
  - `IResourceService`
  - `IJobService`
  - `IMemoryService`
- this should become the formal module contract sheet after ownership and dependency rules are settled
- implementation progress on 2026-04-08:
  - added `src/modules/render/irender_service.hpp`
  - `RenderModule` now implements `IRenderService`
  - `EngineContext` now exposes render access through `IRenderService*`
  - `EngineCore` now stores render access through `IRenderService*` instead of the concrete `RenderModule` type
  - added `src/modules/gpu/igpu_service.hpp`
  - `GpuModule` now implements that interface
  - `EngineContext` now exposes GPU access through `IGpuService*`
  - `RenderModule` now depends on `IGpuService` instead of the concrete `GpuModule` type
  - added `src/modules/resources/iresource_service.hpp`
  - `EngineContext` now exposes resource access through `IResourceService*`
  - `IResourceService` now includes richer inspection/query methods beyond the initial request/state surface
  - `EngineCore` now stores resource access through `IResourceService*` instead of the concrete `ResourceModule` type
  - `IGpuService` now includes a first generic uploaded-resource contract
  - `IResourceService` now exposes GPU upload handle and uploaded-byte inspection for resources
  - added `src/modules/jobs/ijob_service.hpp`
  - added `src/modules/file/ifile_service.hpp`
  - added `src/modules/platform/iplatform_service.hpp`
  - added `src/modules/memory/imemory_service.hpp`
  - `EngineContext` now exposes jobs and files through interface pointers instead of concrete module types
  - `EngineContext` now exposes platform and memory through interface pointers as well
  - `ResourceModule` now depends on `IJobService` and `IFileService` instead of `JobModule` and `FileModule`
  - `InputModule`, `GpuModule`, `WebViewModule`, and `NativeUiModule` now consume `IPlatformService` instead of `PlatformModule`
  - `EngineCore` now stores job access through `IJobService*` instead of `JobModule*`
  - `EngineCore` now stores file access through `IFileService*` instead of `FileModule*`
  - the `EngineCore` header no longer needs to advertise concrete job/file module ownership for those lanes
- status:
  - in progress
  - GPU, render, resource, job, file, platform, and memory service slices now exist
  - the remaining work is now more about deepening those contracts than extracting their first interface layer

## TODO

Task:

- refine `JobModule` so it stays an execution system rather than drifting into ownership

Progress/Note:

- `JobModule` should own:
  - worker threads
  - queues
  - execution policy
  - priorities
  - background dispatch
- it may perform:
  - file reads
  - parsing
  - decoding
  - mesh processing
  - compression/decompression
  - procedural/background work
- it should not own long-term resource state, scene state, or GPU lifetime
- implementation progress on 2026-04-08:
  - `IJobService` and `JobModule` now expose a peak queued-job metric
  - `JobModule` now tracks queue high-water pressure during runtime instead of only current queued count
  - `EngineCore` now exposes that peak queued-job metric and includes it in the debug view

## TODO

Task:

- formalize memory policy categories so future resource and frame systems have a stable allocation model

Progress/Note:

- `MemoryModule` should remain the home of:
  - allocators
  - categories
  - tracking
  - diagnostics
  - budgets
  - transient/persistent policy
- target categories already identified:
  - permanent engine memory
  - persistent resource memory
  - transient frame memory
  - upload/staging memory
  - debug memory
- this becomes more important once render, resource, and upload paths are split cleanly
- implementation progress on 2026-04-08:
  - `MemoryStats` now tracks peak bytes per memory class instead of only a global peak
  - `IMemoryService`, `MemoryModule`, and `EngineCore` now expose per-class peak byte queries
  - the debug view now shows current and peak values for the `Resource`, `Temporary`, and `Debug` memory classes

## TODO

Task:

- document the render-frame flow against the new ownership model

Progress/Note:

- target frame flow:
  - app/scene prepares world state
  - render-facing scene data is gathered
  - `RenderModule` builds render queues and frame data
  - `RenderModule` asks `GpuModule` or backend services to execute GPU work
  - frame presents
- this is useful as a validation check once the GPU/render split starts landing in code

## TODO

Task:

- convert this planning log into an implementation contract sheet once the architecture direction is locked

Progress/Note:

- next document should be more concrete than this one
- it should include:
  - exact allowed dependencies
  - exact ownership boundaries
  - exact interface names and responsibilities
  - transition notes from current code to target structure
- this TODO file should remain the planning log until those contracts are ready
