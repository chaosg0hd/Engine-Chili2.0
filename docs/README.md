# Engine-Chili2.0 Docs

This is the intentionally small documentation surface for the repo.

## Canonical Docs

- [README](./README.md)
  - surface-level project state, build path, and doc map
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
- the public render path is prototype-driven: `FramePrototype -> RenderFrameData`
- the runtime is split into management, logic, and presentation domains
- the sound path is live through `SoundModule`
- the DX11 path is functional, but materials, indirect lighting, and resource maturity are still in progress

## Architecture Snapshot

- apps talk through capabilities and prototype inputs
- Studio project management is isolated under `apps/studio/src/studio/`
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
