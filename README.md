# Engine-Chili2.0
Game Engine from Scratch

Current targets:

- `EngineRuntime` - shared library target that outputs `engine.dll`
- `PongRuntime` - Pong-owned shared library target under `apps/pong` that outputs `pong_runtime.dll`
- `Studio` - native studio host executable, output as `engine_studio.exe`
- `PongPreview` - Pong-owned thin executable under `apps/pong` that loads `pong_runtime.dll`
- `HotBuildTool` - standalone external tool stub under `tools/hotbuild`

Current state:

- the old sandbox executable is no longer part of the top-level build; rapid Pong testing now moves through `apps/pong`'s `PongPreview`
- the front-facing app architecture is being iterated through real game trials, currently using `apps/pong`, future game runtime DLLs, and `apps/_template` to pressure-test what app authors and players see and do
- older one-off sandbox variants have been stripped down; `hex_observation` remains in archive while its debug types/logic migrate into `src/modules/diagnostics/`
- frame submission now flows through `FramePrototype` from app-facing code into the renderer/GPU path
- prototype families now live under:
  - `src/prototypes/presentation/`
  - `src/prototypes/entity/appearance/`
  - `src/prototypes/entity/geometry/`
  - `src/prototypes/entity/object/`
  - `src/prototypes/entity/scene/`
  - `src/prototypes/math/`
- `RenderModule` now translates prototype requests into render-owned `RenderFrameData`
- `GpuModule` and backend internals now consume render-owned data instead of prototype structs directly
- the DX11 backend now realizes submitted frame contents into visible geometry
- the active DX11 sandbox path now supports:
  - point-light shading
  - cubemap shadow scaffolding for the primary point light
  - albedo texture sampling on built-in meshes
  - prototype-driven material tint blending
- `ViewPrototype` can carry light-ray prototypes into render-owned frame data, but that is not the active sandbox lane
- in-window overlay text is currently a separate Win32 paint path, not renderer-owned text

Architecture rule:

- shared engine-side prototypes and built-in material defaults must stay generic
- sandbox experiments, look-dev tweaks, and scene-specific authoring belong in sandbox-owned configuration or dedicated sandbox-specific prototype variants
- app-side experimentation should not silently rewrite shared engine defaults just to prove a feature path

Architecture snapshot:

- `PlatformModule` owns the OS window and render surface
- `GpuModule` owns the backend, device-facing presentation path, and generic GPU resources
- `RenderModule` owns render submission flow, prototype-to-render translation, and frame orchestration
- `ResourceModule` owns engine-facing asset/resource state
- `JobModule` owns worker execution
- `MemoryModule` owns allocation policy and tracking

Render path snapshot:

- sandbox/app code builds a `FramePrototype`
- `EngineCore` forwards it to `RenderModule`
- `RenderModule` translates it into render-owned `RenderFrameData`
- `RenderModule` submits that through `GpuModule`
- the DX11 backend uploads and draws submitted frame items from render-owned data

Build instructions:

Preferred local commands:

```powershell
.\configure.cmd
.\build.cmd
.\build.cmd studio
.\build.cmd pong
.\build.cmd engine
```

Target aliases:

- `studio` builds the `Studio` target and outputs `build/bin/studio/engine_studio.exe`
- `pong` builds `PongPreview` plus the Pong runtime DLL outputs under `build/bin/apps/pong`
- `engine` builds the reusable `EngineRuntime` target and outputs `build/bin/engine/engine.dll`
- no target argument builds the default CMake all target

Agent note:

- In Codex or similar sandboxed agents, run `configure.cmd`, `build.cmd`, direct `cmake`, and Ninja commands only with escalated execution.
- The wrapper commands write logs to `logs/cmake-configure.log` and `logs/cmake-build.log`.

Build direction:

- the project is moving away from hot-building monolithic executables as the normal workflow
- the target runtime shape is:
  - launcher `.exe`
  - `engine.dll`
  - app/editor/game DLLs loaded after the engine
- the launcher should stay thin: process setup, DLL loading, and handoff only
- `engine.dll` should own reusable engine systems and stable module boundaries
- app/tool DLLs should own project-specific runtime/editor behavior without forcing the launcher or engine core to relink for every iteration
- future build work should prefer DLL-safe boundaries, explicit exported entry points, and reload-friendly ownership over direct executable coupling

Raw CMake equivalent:

```powershell
cmake -S . -B build -G Ninja
cmake --build build
```

Current output layout:

```txt
build/bin/engine/engine.dll
build/bin/studio/engine_studio.exe
build/bin/apps/pong/PongPreview.exe
build/bin/apps/pong/pong_runtime.dll
build/bin/tools/hotbuild/HotBuildTool.exe
```

Codex note:

- In this repo, build commands must be run outside the normal CLI sandbox.
- Codex may need to run `cmake -S . -B build -G Ninja` and `cmake --build build` outside the sandbox because CMake try-compile/build subprocesses can stall or fail under sandboxed execution.
- As the DLL build model lands, documentation and code changes should keep the launcher/engine/app-DLL split explicit instead of folding new behavior into one executable target.

GitHub build note:

- the GitHub Actions Windows build should use the Visual Studio/MSVC toolchain with Ninja
- this repo is heavily Win32/DX11/WebView2-oriented, so MSVC is the intended CI compiler lane rather than MSYS2/MinGW

Sanitizer build:

```powershell
Remove-Item -Recurse -Force build\sanitize -ErrorAction SilentlyContinue
cmake -S . -B build\sanitize -G Ninja -DENABLE_SANITIZERS=ON
cmake --build build\sanitize
```

Studio status:

- `Studio` outputs `engine_studio.exe`, keeps the existing native engine window, and hosts an embedded WebView2 sidebar
- The first embedded tool surface lives under `apps/studio/coretools`
- The current milestone is Windows-only and focused on a fixed left-docked CoreTools surface
- Future HTTP and WebSocket transport code remains under `apps/studio/src/transport`, but it is not the active runtime path for this milestone

Sandbox status:

- the active sandbox is a DX lighting/material lab under `apps/sandbox/`
- the sandbox app configures a lighting-lab scene preset, camera, and one primary point light
- the current sandbox scene is a meter-based room with a rotating cube, stucco-backed material prototypes, and live exposure/light controls
- current visible material support is:
  - albedo texture sampling
  - prototype color/tint blending over albedo
  - roughness/specular controls lowered into the DX11 shader path
- current shadow support is:
  - first-pass cubemap shadow generation for the primary point light
  - basic shadow sampling in the scene shader
- normal and height maps are declared in material prototypes but are not yet part of final shading

See [docs/README.md](docs/README.md) for the current architecture, feature list, and API inventory.
