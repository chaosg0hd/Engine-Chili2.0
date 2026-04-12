# Engine-Chili2.0
Game Engine from Scratch

Current targets:

- `engine_core` - reusable engine library
- `engine_sandbox` - active sandbox app under `apps/sandbox/`
- `engine_studio` - native studio host with an embedded WebView2 CoreTools surface

Current state:

- `engine_sandbox` now owns the active sandbox harness under `apps/sandbox/src/`
- the older broad sandbox harness is preserved under `apps/sandbox/archive/`
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
- `ViewPrototype` can now carry light-ray prototypes into render-owned frame data
- in-window overlay text is currently a separate Win32 paint path, not renderer-owned text

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

Run instructions:

```powershell
Remove-Item -Recurse -Force build -ErrorAction SilentlyContinue
cmake -S . -B build -G Ninja
cmake --build build
```

Codex note:

- In this repo, direct CMake commands are the reliable path.
- Codex may need to run `cmake -S . -B build -G Ninja` and `cmake --build build` outside the sandbox because CMake try-compile/build subprocesses can stall or fail under sandboxed execution.

Sanitizer build:

```powershell
Remove-Item -Recurse -Force build\sanitize -ErrorAction SilentlyContinue
cmake -S . -B build\sanitize -G Ninja -DENABLE_SANITIZERS=ON
cmake --build build\sanitize
```

Studio status:

- `engine_studio` keeps the existing native engine window and hosts an embedded WebView2 sidebar
- The first embedded tool surface lives under `apps/studio/coretools`
- The current milestone is Windows-only and focused on a fixed left-docked CoreTools surface
- Future HTTP and WebSocket transport code remains under `apps/studio/src/transport`, but it is not the active runtime path for this milestone

Sandbox status:

- the active sandbox is currently a prototype-driven light ray lab
- it builds a dark DX11 scene with a visible light source, ray markers, receiver geometry, and a submitted `LightPrototype`
- it uses `presentation`, organized `entity`, and `math` prototype families
- temporary built-in test geometry is currently owned by the sandbox in `apps/sandbox/src/sandbox_builtin_meshes.hpp`
- it is now a proof path for light/ray prototype data flow; true light simulation in the renderer is still future work

See [docs/README.md](docs/README.md) for the current architecture, feature list, and API inventory.
