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
- rendering is still closer to a software-framebuffer/runtime harness than a fully separated GPU-driven architecture
- GPU capability work exists, but a true generic `GpuModule` boundary has not been established yet
- resource ownership is not yet formalized into a dedicated `ResourceModule`
- the current project is in a transition stage from feature scaffolding into clearer production-oriented ownership boundaries

Target direction:

- platform provides surfaces
- GPU owns device resources
- renderer draws frames
- resources own asset state
- jobs perform work
- memory defines allocation policy

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

## DONE

Task:

- define the dependency direction rules between core modules

Progress/Note:

- finished on 2026-04-08
- the dependency direction rules are now documented in [README.md](c:/Users/Neilwinn%20Pineda/Documents/Engine-Chili2.0/docs/engine/README.md)
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
- the ownership rules are now documented in [README.md](c:/Users/Neilwinn%20Pineda/Documents/Engine-Chili2.0/docs/engine/README.md)
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
- the render-surface handoff is now documented in [README.md](c:/Users/Neilwinn%20Pineda/Documents/Engine-Chili2.0/docs/engine/README.md)
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
  - added [`GpuModule`](c:/Users/Neilwinn%20Pineda/Documents/Engine-Chili2.0/src/modules/gpu/gpu_module.hpp) and [`gpu_module.cpp`](c:/Users/Neilwinn%20Pineda/Documents/Engine-Chili2.0/src/modules/gpu/gpu_module.cpp)
  - moved render-backend ownership, viewport tracking, resize handling, and frame presentation into `GpuModule`
  - `EngineCore` now creates `GpuModule` before `RenderModule`
  - graphics backend naming now comes from `GpuModule` instead of the compute stub path
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
  - backend ownership is now in `GpuModule`, but generic GPU resource services are still not exposed beyond the current frame-render path

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
  - added [`ResourceModule`](c:/Users/Neilwinn%20Pineda/Documents/Engine-Chili2.0/src/modules/resources/resource_module.hpp) and [`resource_module.cpp`](c:/Users/Neilwinn%20Pineda/Documents/Engine-Chili2.0/src/modules/resources/resource_module.cpp)
  - added resource handles, resource kinds, and resource load-state tracking
  - registered `ResourceModule` in `EngineCore`
  - exposed first resource wrappers through `EngineCore` for request, unload, state query, and tracked-count query
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
  - the state machine and handle table exist, but file/job/gpu handoff is not wired yet

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
- this flow is a strong reference model for future mesh/material/shader loading too

## TODO

Task:

- clarify renderer-private resources versus general GPU resources

Progress/Note:

- current intended split:
  - `GpuModule` owns generic buffers, textures, samplers, shader objects, views, upload/readback buffers
  - `RenderModule` owns backbuffer views, depth targets, frame attachments, pass-temporary targets, and render-only constant buffers
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
  - added [`IGpuService`](c:/Users/Neilwinn%20Pineda/Documents/Engine-Chili2.0/src/modules/gpu/igpu_service.hpp)
  - `GpuModule` now implements that interface
  - `EngineContext` now exposes GPU access through `IGpuService*`
  - `RenderModule` now depends on `IGpuService` instead of the concrete `GpuModule` type
- status:
  - in progress
  - only the GPU service slice exists so far

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
