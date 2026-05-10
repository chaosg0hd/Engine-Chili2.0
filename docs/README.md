# Engine-Chili2.0 Docs

This is the intentionally small documentation surface for the repo.

## Canonical Docs

- [README](./README.md)
  - surface-level project state, build path, and doc map
- [Build Lanes](./build/BUILD_LANES.md)
  - current multi-lane build direction and convergence target
- [API Map](./engine/API_MAP.md)
  - practical map of the engine-facing call surfaces
- [TODO](./engine/TODO.md)
  - architecture progress log and next structural work
- [ARCHI_RULES](./engine/ARCHI_RULES)
  - ownership and boundary contract
- [Visual Language](./css/visual-language.md)
  - UI direction
- [theme-light.css](./css/theme-light.css)
  - single retained docs theme

## Project Snapshot

- the build/runtime direction is moving to a thin launcher executable plus DLL-loaded engine and app/tool modules
- `EngineRuntime` is the reusable native runtime DLL target
- `apps/pong` owns the first app/game DLL target, `PongRuntime`
- `Studio` is the native studio host executable target
- `apps/pong` owns the first preview host, `PongPreview`
- `HotBuildTool` is a standalone external tool stub
- Studio now owns the first project-management workflow: File dialog, New/Open/Save project actions, and a right-docked Project Explorer
- Studio now has a centralized interaction layer for viewport tools, selection, and inspector updates
- Studio now uses one authoritative viewport rectangle for render, camera aspect, and picking alignment
- an initial reusable `InputSystem` prototype now exists under `src/input/` and is integrated in Studio via named action contexts
- a first-pass Proxy Library migration is active in Studio:
  - project `assetProxyFolder` configuration in `project.chili.json`
  - proxy scan + registry write to `.chili/asset_registry.json`
  - reusable prototype entries (`proto.default_cube`, `proto.default_light`)
  - runtime object instances reference prototypes
  - scene object-instance format supports `prototype`, `values`, and `overrides`
- the public render path is prototype-driven: `FramePrototype -> RenderFrameData`
- the runtime is split into management, logic, and presentation domains
- the sound path is live through `SoundModule`
- the DX11 path is functional, but materials, indirect lighting, and resource maturity are still in progress

## Architecture Snapshot

- prototype definition:
  - in this engine, a prototype is a reusable construction object, not a disposable experiment and not a passive data-only struct
  - prototypes may contain methods, lifecycle hooks, and chaining/composition logic
  - prototypes define reusable construction behavior implemented once and reused later
- apps talk through capabilities and prototype inputs
- Studio project management is isolated under `apps/studio/src/studio/`
- Studio layout owns viewport rect computation in one place, and runtime/editor consumers read that shared result
- Studio filesystem access goes through `FileProxy`, with user projects rooted under `User/<project_id>/`
- modules own behavior, lifetime, and translation into private execution state
- `RenderModule` owns frame orchestration
- `GpuModule` owns backend/device lifetime and generic GPU handles
- `ResourceModule` owns logical resource state and decoded CPU payloads
- `EngineCore` is still the integration root, but it is not the intended long-term home for feature logic

Read [ARCHI_RULES](./engine/ARCHI_RULES) before changing ownership, boundaries, or app-facing APIs.

## Build

Direction:

- hot-building monolithic executables is no longer the desired long-term workflow
- the intended binary shape is `launcher.exe -> engine.dll -> app/tool/runtime DLLs`
- the launcher owns process startup and dynamic loading only
- `engine.dll` owns reusable engine systems
- app/tool DLLs own project-specific behavior and should be reload-friendly where possible
- app-specific runtime and preview targets should live with the app folder they belong to
- future CMake and runtime work should preserve this split instead of adding more direct executable coupling

Current build-lane direction:

- build entry points are currently transitional across CI, agent, Studio, and human workflows
- see [Build Lanes](./build/BUILD_LANES.md) for supported lanes and the convergence target
- eventual goal is a unified builder contract that all lanes call into, rather than lane-specific policy copies

```powershell
Remove-Item -Recurse -Force build -ErrorAction SilentlyContinue
cmake -S . -B build -G Ninja
cmake --build build
```

Current output layout:

```txt
build/bin/engine/
build/bin/studio/
build/bin/apps/pong/
build/bin/tools/hotbuild/
```

Sanitizer build:

```powershell
Remove-Item -Recurse -Force build\sanitize -ErrorAction SilentlyContinue
cmake -S . -B build\sanitize -G Ninja -DENABLE_SANITIZERS=ON
cmake --build build\sanitize
```

## Repo Notes

- build/configure/test commands must be run outside the normal Codex CLI sandbox for this Windows/DX-oriented repo
- prefer the launcher/DLL split for new build architecture work
- expected CI lane is Windows + MSVC + Ninja
- move durable feature summaries and contracts into `README`, `API_MAP`, `TODO`, or `ARCHI_RULES`
- `User/` is generated Studio workspace data and should not be committed
